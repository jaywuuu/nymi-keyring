#include "nymi.h"
#include <vector>

static bool gNCLInitialized = false;

static std::vector<NymiDevice> gNymiDevices;

NymiState gNymiState;

NymiResultCode nymiInit(bool emu, const char *deviceName, const char *ip = "127.0.0.1", int port = 9089) {
	gNymiState.nymiDevice = 0; // defaults to 0th device
	gNymiState.validated = false;
	gNymiState.state = NYMI_OK;
	memset(gNymiState.leds, 0, sizeof(NclBool)*NCL_AGREEMENT_PATTERNS*NCL_LEDS);
	gNymiState.sk = NULL;
	gNymiState.sk_id = NULL;

	/* load saved nymi devices */

	/* if nothing loaded, init one empty nymi device and add to gNymiDevices */
	if (gNymiDevices.size() == 0) {
		NymiDevice device = {
			"nymi",
			-1,
			false,
			{ 0, 0, false}
		};
		gNymiDevices.push_back(device);
	}

	if (emu) {
		if (!nclSetIpAndPort(ip, port)) return NYMI_SET_IP_FAIL;
	}

	if (!nclInit(nymiEventHandler, &gNymiState, deviceName, NCL_MODE_DEFAULT, stderr)) return NYMI_INIT_FAIL;

	return NYMI_OK;
}

void nymiEventHandler(NclEvent event, void *retState) {
	int nymiDevice = ((NymiState *)retState)->nymiDevice;
	NclBool res;
	switch (event.type) {
	case NCL_EVENT_INIT:
		if (event.init.success) gNCLInitialized = true;
		else {
			((NymiState *)retState)->state = NYMI_INIT_FAIL;
			return;
		}
		break;
	case NCL_EVENT_ERROR:
		exit(-1);
		break;
	case NCL_EVENT_DISCONNECTION:
		gNymiDevices[nymiDevice].handle = -1;
		gNymiState.state = NYMI_DISCONNECTED;
		break;
	case NCL_EVENT_DISCOVERY:
		/* on discovery,  stop scan and initiate agree request */
		res = nclStopScan();
		if (!res) {
			((NymiState *)retState)->state = NYMI_STOPSCAN_FAIL;
			return;
		}
		gNymiDevices[nymiDevice].handle = event.discovery.nymiHandle;
		res = nclAgree(gNymiDevices[nymiDevice].handle);
		if (!res) {
			((NymiState *)retState)->state = NYMI_AGREE_REQ_FAIL;
			return;
		}
		((NymiState *)retState)->state = NYMI_WAIT;
		return;
		break;
	case NCL_EVENT_FIND:
		/* once found, set the handle and validate */
		res = nclStopScan();
		if (!res) {
			((NymiState*)retState)->state = NYMI_STOPSCAN_FAIL;
			return;
		}
		gNymiDevices[nymiDevice].handle = event.find.nymiHandle;
		res = nclValidate(gNymiDevices[nymiDevice].handle);
		if (!res) {
			((NymiState*)retState)->state = NYMI_VALIDATE_REQ_FAIL;
			return;
		}
		((NymiState*)retState)->state = NYMI_WAIT;
		return;
		break;
	case NCL_EVENT_AGREEMENT:
		memcpy(((NymiState*)retState)->leds, event.agreement.leds, NCL_AGREEMENT_PATTERNS*NCL_LEDS*sizeof(NclBool));
		break;
	case NCL_EVENT_PROVISION:
		memcpy(&gNymiDevices[nymiDevice].provis, &event.provision.provision, sizeof(NclProvision));
		gNymiDevices[nymiDevice].provisioned = true;
		break;
	case NCL_EVENT_VALIDATION:
		((NymiState*)retState)->validated = true;
		break;
	case NCL_EVENT_CREATED_SK:
		if (event.createdSk.nymiHandle != gNymiDevices[nymiDevice].handle) {
			((NymiState*)retState)->state = NYMI_HANDLE_NOT_MATCH;
			return;
		}
		if (((NymiState*)retState)->sk_id == NULL) {
			((NymiState*)retState)->state = NYMI_UNINIT_SK_ID;
			return;
		}
		memcpy(((NymiState*)retState)->sk_id, event.createdSk.id, SK_ID_TOTAL_BYTES*sizeof(char));
		break;
	case NCL_EVENT_GOT_SK:
		if (event.gotSk.nymiHandle != gNymiDevices[nymiDevice].handle) {
			((NymiState*)retState)->state = NYMI_HANDLE_NOT_MATCH;
			return;
		}
		if (((NymiState*)retState)->sk == NULL) {
			((NymiState*)retState)->state = NYMI_UNINIT_SK;
			return;
		}
		memcpy(((NymiState*)retState)->sk, event.createdSk.id, SK_TOTAL_BYTES*sizeof(char));
		break;
	default:
		break;
	}

	((NymiState*)retState)->state = NYMI_OK;
}

bool getNCLInitState() {
	return gNCLInitialized;
}

NymiDevice *getNymiDevicePtr(unsigned int deviceNumber) {
	if (deviceNumber < 0 || deviceNumber > gNymiDevices.size() - 1)
		return NULL;

	return &(gNymiDevices[deviceNumber]);
}

int nymiAddDevice(int device) {
	gNymiState.nymiDevice = device;
	gNymiState.state = NYMI_WAIT;
	NclBool res = nclStartDiscovery();
	if (!res) return NYMI_DISCOVERY_FAIL;
	waitForNymi();
	return NYMI_OK;
}

int nymiProvision(int device) {
	NclBool res = nclProvision(gNymiDevices[device].handle, NCL_FALSE);
	if (!res) return NYMI_PROVISION_FAIL;
	return NYMI_OK;
}

int nymiValidate(int device) {
	NclBool res = nclStartFinding(&gNymiDevices[device].provis, 1, NCL_FALSE);
	if (!res) NYMI_FIND_FAIL;
	waitForNymi();
	if (gNymiState.state == NYMI_OK && gNymiState.validated == true) return NYMI_OK;
	else return NYMI_VALIDATE_FAIL;
}

bool nymiIsConnected(int device) {
	if (gNymiDevices[device].handle == -1) return false;
	return true;
}

int waitForNymi() {
	int counter = 0;
	while (gNymiState.state == NYMI_WAIT) counter++;
	return counter;
}

int nymiCreateSK(int device, byte *id_out) {
	gNymiState.sk_id = id_out;
	int res = nclCreateSk(gNymiDevices[device].handle);
	if (!res) return NYMI_CREATE_SK_FAIL;
	waitForNymi();
	if (gNymiState.state == NYMI_HANDLE_NOT_MATCH) return NYMI_HANDLE_NOT_MATCH;

	return NYMI_OK;
}

int nymiGetSk(int device, byte *id, byte *key_out) {
	gNymiState.sk = key_out;
	int res = nclGetSk(gNymiDevices[device].handle, id);
	if (!res) return NYMI_GET_SK_FAIL;
	waitForNymi();
}