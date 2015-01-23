#ifndef _NYMI_H_
#define _NYMI_H_

#include "ncl.h"
#include "keytable.h"
#include <string>

typedef struct {
	std::string name;
	int handle;
	bool provisioned;
	NclProvision provis;
} NymiDevice;

typedef struct {
	/* input into handler */
	int nymiDevice;
	bool validated;

	/* return values */
	int state;
	NclBool leds[NCL_AGREEMENT_PATTERNS][NCL_LEDS];
	byte *sk_id;
	byte *sk;
} NymiState;

typedef enum {
	NYMI_OK = 0,
	NYMI_WAIT,
	NYMI_DISCONNECTED,
	NYMI_SET_IP_FAIL,
	NYMI_INIT_FAIL,
	NYMI_STOPSCAN_FAIL,
	NYMI_AGREE_REQ_FAIL,
	NYMI_VALIDATE_REQ_FAIL,
	NYMI_VALIDATE_FAIL,
	NYMI_DISCOVERY_FAIL,
	NYMI_FIND_FAIL,
	NYMI_PROVISION_FAIL, 
	NYMI_CREATE_SK_FAIL,
	NYMI_HANDLE_NOT_MATCH,
	NYMI_GET_SK_FAIL,
	NYMI_UNINIT_SK_ID,
	NYMI_UNINIT_SK,

	NYMI_MAXEVENT
} NymiResultCode;

extern NymiState gNymiState;

NymiResultCode nymiInit(bool emu, const char *deviceName, const char *ip = "127.0.0.1", int port = 9089);
void nymiEventHandler(NclEvent event, void *userData);
int waitForNymi();
bool getNCLInitState();
NymiDevice *getNymiDevicePtr(unsigned int deviceNumber);
int nymiAddDevice(int device);
int nymiProvision(int device);
int nymiValidate(int device);
bool nymiIsConnected(int device);
int nymiCreateSK(int device, byte *id_out);
int nymiGetSk(int device, byte *id, byte *key_out);

#endif