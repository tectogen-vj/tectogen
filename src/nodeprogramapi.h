#ifndef NODE_PROGRAM_API_INCLUDED
#define NODE_PROGRAM_API_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef enum NodeProgramPortRole {
  NodeProgramPortRoleInput=1,
  NodeProgramPortRoleOutput=2,
  NodeProgramPortRoleControl=3,
  NodeProgramPortRoleDisplay=4
} NodeProgramPortRole;

typedef enum NodeProgramPortType {
  NodeProgramPortTypeScalar=8,
  NodeProgramPortTypeSampleBlock=9,
  NodeProgramPortTypeSpectrum=10,
  NodeProgramPortTypeShader=11
} NodeProgramPortType;

typedef void* NodeProgramHandle;

typedef struct NodeProgramPortDescriptor {
  const char* name;
  NodeProgramPortRole role;
  NodeProgramPortType type;
} NodeProgramPortDescriptor;

typedef struct NodeProgramPortState {
  void** payload;
} NodeProgramPortState;

typedef struct NodeProgramDescriptor NodeProgramDescriptor; // foward decl.

typedef struct NodeProgramState {
  NodeProgramPortState* portState;
  void* userdata;
  const NodeProgramDescriptor* descriptor;
  const int instanceId;
} NodeProgramState;

typedef struct NodeProgramDescriptor {
  const char * identifier;
  unsigned int portCount;
  const NodeProgramPortDescriptor* portDescriptors;
  void* userdata;
  NodeProgramHandle (*instantiate)(const NodeProgramDescriptor* descriptor);
  void (*invoke)(NodeProgramHandle instance, NodeProgramState* state);
} NodeProgramDescriptor;

typedef const NodeProgramDescriptor* (*NodeProgramDescriptorLoader)(unsigned long index);

#ifdef __cplusplus
}
#endif

#endif // NODE_PROGRAM_API_INCLUDED
