#ifndef NODE_PROGRAM_API_INCLUDED
#define NODE_PROGRAM_API_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

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

typedef void* tn_Handle;

typedef struct tn_PortDescriptor {
  const char* name;
  tn_PortRole role;
  tn_PortType type;
} tn_PortDescriptor;

typedef struct tn_PortState {
  void** payload;
} tn_PortState;

typedef struct tn_Descriptor tn_Descriptor; // foward decl.

typedef struct tn_State {
  tn_PortState* portState;
  void* userdata;
  const tn_Descriptor* descriptor;
  const int instanceId;
} tn_State;

typedef struct tn_Descriptor {
  const char * identifier;
  unsigned int portCount;
  const tn_PortDescriptor* portDescriptors;
  void* userdata;
  tn_Handle (*instantiate)(const tn_Descriptor* descriptor);
  void (*invoke)(tn_Handle instance, tn_State* state);
} tn_Descriptor;

typedef const tn_Descriptor* (*tn_DescriptorLoader)(unsigned long index);

#ifdef __cplusplus
}
#endif

#endif // NODE_PROGRAM_API_INCLUDED
