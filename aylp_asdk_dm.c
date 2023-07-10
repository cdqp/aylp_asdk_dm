#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anyloop.h"
#include "logging.h"
#include "aylp_asdk_dm.h"

#include "asdkWrapper.h"


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
	log_info("Seeing %u actuators", data->n_act);

	// set types
	self->type_in = AYLP_T_BLOCK | AYLP_U_MINMAX;
	self->type_out = 0;

	return 0;
}


int aylp_asdk_dm_process(struct aylp_device *self, struct aylp_state *state)
{
	struct aylp_asdk_dm_data *data = self->device_data;
	/*	we can't do this yet because we still have to pack from the
	*	11x11 from vonkarman_stream to the 97, which will be slow as
	*	balls. for now we ignore it.
	if (state->block->size != data->n_act) {
		log_error("Block size %llu doesn't match n_act %lu",
			state->block->size, data->n_act
		);
		return -1;
	}
	*/
	for (size_t i = 0; i < state->block->size; i++) {
		state->block->data[i] *= 0.01;
	}
	// TODO: check return value?
	asdkSend(data->dm, state->block->data);
	return 0;
}


int aylp_asdk_dm_close(struct aylp_device *self)
{
	struct aylp_asdk_dm_data *data = self->device_data;
	// asdkReset will segfault if it didn't find libait earlier lol
	if (data->n_act) asdkReset(data->dm);
	asdkRelease(data->dm); data->dm = 0;
	free(data->sn); data->sn = 0;
	free(data); self->device_data = 0;
	return 0;
}

