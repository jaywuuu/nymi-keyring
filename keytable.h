/* storing keys */

#ifndef _KEY_TABLE_H_
#define _KEY_TABLE_H_

#define SK_ID_TOTAL_BYTES		16
#define SK_TOTAL_BYTES			16

typedef unsigned char byte;

typedef struct {
	int appId;
	int nymiDevice;
	byte sk_id[SK_ID_TOTAL_BYTES];
	byte sk[SK_TOTAL_BYTES];
} SKEntry;

extern std::vector<SKEntry> SKTable;

#endif