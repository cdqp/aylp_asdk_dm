#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gsl/gsl_block.h>

#include "anyloop.h"
#include "block.h"
#include "logging.h"
#include "aylp_asdk_dm.h"

#include "asdkWrapper.h"


// returns length of mat_is
static size_t parse_mat_indices(size_t **mat_is, json_object *obj)
{
	if (!json_object_is_type(obj, json_type_array)) {
		log_error("Object with key mat_is/mat_js must be array");
		return 0;
	}
	size_t len = json_object_array_length(obj);
	*mat_is = malloc(len * sizeof(size_t));
	if (!*mat_is) {
		log_error("Couldn't malloc: %s", strerror(errno));
		return 0;
	}
	for (size_t i = 0; i < len; i++) {
		errno = 0;
		(*mat_is)[i] = json_object_get_uint64(
			json_object_array_get_idx(obj, i)
		);
		if (errno) {
			log_error("Couldn't convert value at index %zu: %s",
				i, strerror(errno)
			);
		}
	}
	return len;
}


int aylp_asdk_dm_init(struct aylp_device *self)
{
	int err;
	self->device_data = (struct aylp_asdk_dm_data *)calloc(
		1, sizeof(struct aylp_asdk_dm_data)
	);
	struct aylp_asdk_dm_data *data = self->device_data;
	// attach methods
	self->process = &aylp_asdk_dm_process;
	self->close = &aylp_asdk_dm_close;
	data->mat_is = 0; data->mat_is_len = 0;
	data->mat_js = 0; data->mat_js_len = 0;

	// parse the params json into our data struct
	if (!self->params) {
		log_error("No params object found.");
		return -1;
	}
	json_object_object_foreach(self->params, key, val) {
		if (key[0] == '_') {
			// keys starting with _ are comments
		} else if (!strcmp(key, "sn")) {
			data->sn = strdup(json_object_get_string(val));
			log_trace("sn = %s", data->sn);
		} else if (!strcmp(key, "peak_per_rad")) {
			data->peak_per_rad = json_object_get_double(val);
			log_trace("peak_per_rad = %G", data->peak_per_rad);
		} else if (!strcmp(key, "mat_is")) {
			size_t len = parse_mat_indices(&data->mat_is, val);
			log_trace("got mat_is of length = %zu", len);
			data->mat_is_len = len;
		} else if (!strcmp(key, "mat_js")) {
			size_t len = parse_mat_indices(&data->mat_js, val);
			log_trace("got mat_js of length = %zu", len);
			data->mat_js_len = len;
		} else {
			log_warn("Unknown parameter \"%s\"", key);
		}
	}
	// make sure we didn't miss any params
	if (!data->sn) {
		log_error("You must provide the sn parameter.");
		return -1;
	}

	data->dm = asdkInit(data->sn);
	if (!data->dm) {
		log_error("Failed to initialize DM:");
		asdkPrintLastError();
		return -1;
	}
	log_info("asdkInit completed");

	double tmp = 0.0;
	err = asdkGet(data->dm, "NbOfActuator", &tmp);
	if (err) {
		log_error("Failed to get number of actuators:");
		asdkPrintLastError();
		return -1;
	}
	data->n_act = (size_t)tmp;
	log_info("Seeing %zu actuators", data->n_act);
	err = asdkSet(data->dm, "NbSteps", 1.0);
	if (err) {
		log_error("Failed to set number of steps:");
		asdkPrintLastError();
		return -1;
	}
	// TODO: do we need to set mcff? how? cast double * to Scalar? gross.

	// allocate our send buffer (this is only needed if the input type is
	// not a contiguous AYLP_T_VECTOR)
	data->send_buf = gsl_block_calloc(data->n_act);
	if (!data->send_buf) {
		log_error("Couldn't allocate block: %s", strerror(errno));
		return -1;
	}

	// set types
	self->type_in = AYLP_T_VECTOR | AYLP_T_MATRIX;
	self->units_in = AYLP_U_MINMAX | AYLP_U_RAD;
	self->type_out = 0;
	self->units_out = 0;

	return 0;
}


int aylp_asdk_dm_process(struct aylp_device *self, struct aylp_state *state)
{
	struct aylp_asdk_dm_data *data = self->device_data;
	if (LIKELY(state->header.type == AYLP_T_VECTOR)) {
		gsl_vector *v = state->vector;	// brevity
		if (LIKELY(v->stride == 1)) {
			if (LIKELY(state->header.units == AYLP_U_MINMAX)) {
				// good; we can send it right away and be done
				asdkSend(data->dm, v->data);
				return 0;
			} else {
				log_trace("Pipeline data is vector of radians; "
					"we have to copy from it, which is slow"
				);
				memcpy(data->send_buf->data, v->data,
					sizeof(double) * v->size
				);
			}
		} else {
			// gotta make it contiguous first
			log_trace("Pipeline data vector is non-contiguous; "
				"we have to copy from it, which is slow"
			);
			for (size_t i = 0; i < state->vector->size; i++) {
				data->send_buf->data[i] = v->data[
					i * state->vector->stride
				];
			}
		}
	} else if (state->header.type == AYLP_T_MATRIX) {
		if (UNLIKELY(data->mat_is_len < state->matrix->size1)) {
			log_error("Refusing to index into mat_is with length "
				"of only %zu when we have %zu actuators",
				data->mat_is_len, data->n_act
			);
			return -1;
		}
		if (UNLIKELY(data->mat_js_len < state->matrix->size2)) {
			log_error("Refusing to index into mat_js with length "
				"of only %zu when we have %zu actuators",
				data->mat_js_len, data->n_act
			);
			return -1;
		}
		log_trace("Indexing into pipeline matrix by mat_is and mat_js");
		for (size_t i = 0; i < data->n_act; i++) {
			errno = 0;
			double tmp = gsl_matrix_get(state->matrix,
				data->mat_is[i], data->mat_js[i]
			);
			if (!tmp && errno) {
				log_warn("Couldn't index matrix at (%zu,%zu):"
					" %s", data->mat_is[i], data->mat_js[i],
					strerror(errno)
				);
			}
			data->send_buf->data[i] = tmp;
		}
	} else {
		log_error("Bug: unsupported type 0x%hhX", state->header.type);
		return -1;
	}
	// convert from rad to minmax
	if (state->header.units == AYLP_U_RAD) {
		for (size_t i = 0; i < data->send_buf->size; i++) {
			data->send_buf->data[i] *= data->peak_per_rad;
		}
	}
	// send the recently made-contiguous data and clear the buffer
	asdkSend(data->dm, data->send_buf->data);
	return 0;
}


int aylp_asdk_dm_close(struct aylp_device *self)
{
	struct aylp_asdk_dm_data *data = self->device_data;
	// asdkReset will segfault if it didn't find libait earlier lol
	if (data->n_act) asdkReset(data->dm);
	asdkRelease(data->dm); data->dm = 0;
	gsl_block_free(data->send_buf); data->send_buf = 0;
	free(data->sn); data->sn = 0;
	free(data->mat_is); data->mat_is = 0;
	free(data->mat_js); data->mat_js = 0;
	free(data); self->device_data = 0;
	return 0;
}

