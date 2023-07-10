#ifndef _AYLP_ASDK_DM_H
#define _AYLP_ASDK_DM_H

#include "anyloop.h"

#include "asdkWrapper.h"

struct aylp_asdk_dm_data {
	char *sn;	// serial number
	asdkDM *dm;	// opaque dm object from asdk
	size_t n_act;	// number of actuators
};

// initialize asdk
int aylp_asdk_dm_init(struct aylp_device *self);

// send gsl_block of data in state
int aylp_asdk_dm_process(struct aylp_device *self, struct aylp_state *state);

// close device when loop exits
int aylp_asdk_dm_close(struct aylp_device *self);

#endif

