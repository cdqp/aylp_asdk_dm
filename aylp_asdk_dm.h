#ifndef AYLP_ASDK_DM_H_
#define AYLP_ASDK_DM_H_

#include <gsl/gsl_block.h>

#include "anyloop.h"

#include "asdkWrapper.h"

struct aylp_asdk_dm_data {
	// param: serial number
	char *sn;
	// opaque dm object from asdk
	asdkDM *dm;
	// number of actuators
	size_t n_act;
	// param: matrix row indices corresponding to command; starts from 0
	size_t *mat_is;
	size_t mat_is_len;
	// param: matrix column indices corresponding to command; starts from 0
	size_t *mat_js;
	size_t mat_js_len;
	// param: 1 / peak-to-valley stroke of mirror in radians
	// (i.e. 2/pvstroke/2, because of reflection)
	double peak_per_rad;
	// buffer to send (only used if we're not handed a contiguous command
	// vector)
	gsl_block *send_buf;
};

// initialize asdk
int aylp_asdk_dm_init(struct aylp_device *self);

// send gsl_block of data in state
int aylp_asdk_dm_process(struct aylp_device *self, struct aylp_state *state);

// close device when loop exits
int aylp_asdk_dm_close(struct aylp_device *self);

#endif

