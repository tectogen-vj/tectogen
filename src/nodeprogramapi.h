#ifndef NODE_PROGRAM_API_INCLUDED
#define NODE_PROGRAM_API_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef float tn_Complex[2];

typedef struct tn_FrequencyWindow {
  tn_Complex* buffer;
} tn_FrequencyWindow;

typedef struct tn_TimeWindow {
  float* buffer;
} tn_TimeWindow;

typedef struct tn_Scalar {
	double* v;
} tn_Scalar;

typedef union {
  tn_FrequencyWindow frequency_window;
  tn_TimeWindow time_window;
  tn_Scalar scalar;
	void* raw;
} tn_PortMessage;

typedef struct tn_PortBuffer {
  tn_PortMessage* ring_buffer;
  size_t count;
} tn_PortBuffer;


typedef enum tn_PortRole {
  tn_PortRoleInput=1,
  tn_PortRoleOutput=2,
  tn_PortRoleControl=3,
  tn_PortRoleDisplay=4
} tn_PortRole;

typedef enum tn_PortType {
  tn_PortTypeScalar=8,
  tn_PortTypeSampleBlock=9,
  tn_PortTypeSpectrum=10,
  tn_PortTypeShader=11
} tn_PortType;

typedef void* tn_Userdata;

typedef struct tn_PortDescriptor {
  const char* name;
  tn_PortRole role;
  tn_PortType type;
} tn_PortDescriptor;

typedef struct tn_PortState {
  void** payload_symbol_to_be_obsoleted;
  tn_PortBuffer portData;
} tn_PortState;

typedef struct tn_Config {
  const int windowSize;
  const int freqBins; // windowSize/2+1
  const int lookbackCount;
} tn_Config;

typedef struct tn_Descriptor tn_Descriptor; // foward decl.

typedef struct tn_State {
  tn_PortState* portState;
  tn_Userdata* userdata; // FIXME: populate
  const tn_Descriptor* descriptor;
  const tn_Config* config;
  const int instanceId;
} tn_State;

typedef tn_Userdata (*tn_instantiate_function)(const tn_Descriptor* descriptor, const tn_Config* config);
typedef void (*tn_invoke_function)(tn_State* state, unsigned long idx);

typedef struct tn_Descriptor {
  const char * identifier;
  unsigned int portCount;
  const tn_PortDescriptor* portDescriptors;
  tn_instantiate_function instantiate; // FIXME: call it!
  tn_invoke_function invoke;
} tn_Descriptor;

typedef const tn_Descriptor* (*tn_DescriptorLoader)(unsigned long index);

inline tn_PortMessage tn_getPM(tn_PortState port, unsigned long idx) {
	unsigned long count=port.portData.count;
	int rIdx=idx%count;
	return port.portData.ring_buffer[rIdx];
}

#ifdef __cplusplus
}
#endif

#endif // NODE_PROGRAM_API_INCLUDED
