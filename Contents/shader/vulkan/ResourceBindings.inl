#ifndef ELAINE_RESOURCE_BINDINGS
#define ELAINE_RESOURCE_BINDINGS
#define ELAINE_GROUP_PerFrame 0
#define ELAINE_GROUP_PerMaterial 1
#define ELAINE_GROUP_PerObject 2
#define ELAINE_GROUP_PerPass 3
#define ELAINE_SLOT_Uniform0 0
#define ELAINE_SLOT_Uniform1 1
#define ELAINE_SLOT_Uniform2 2
#define ELAINE_SLOT_Uniform3 3
#define ELAINE_SLOT_Uniform4 4
#define ELAINE_SLOT_Uniform5 5
#define ELAINE_SLOT_Uniform6 6
#define ELAINE_SLOT_Uniform7 7
#define ELAINE_SLOT_Texture0 8
#define ELAINE_SLOT_Texture1 9
#define ELAINE_SLOT_Texture2 10
#define ELAINE_SLOT_Texture3 11
#define ELAINE_SLOT_Texture4 12
#define ELAINE_SLOT_Texture5 13
#define ELAINE_SLOT_Texture6 14
#define ELAINE_SLOT_Texture7 15
#define ELAINE_SLOT_Texture8 16
#define ELAINE_SLOT_Texture9 17
#define ELAINE_SLOT_Texture10 18
#define ELAINE_SLOT_Texture11 19
#define ELAINE_SLOT_Texture12 20
#define ELAINE_SLOT_Texture13 21
#define ELAINE_SLOT_Texture14 22
#define ELAINE_SLOT_Texture15 23
#define ELAINE_SLOT_Buffer0 24
#define ELAINE_SLOT_Buffer1 25
#define ELAINE_SLOT_Buffer2 26
#define ELAINE_SLOT_Buffer3 27
#define ELAINE_SLOT_Buffer4 28
#define ELAINE_SLOT_Buffer5 29
#define ELAINE_SLOT_Buffer6 30
#define ELAINE_SLOT_Buffer7 31
#define ELAINE_UNIFORM(Group, Slot, Name) layout(std140, set = ELAINE_GROUP_##Group, binding = ELAINE_SLOT_##Slot) uniform Name
#define ELAINE_TEXTURE2D(Group, Slot, Name) layout(set = ELAINE_GROUP_##Group, binding = ELAINE_SLOT_##Slot) uniform sampler2D Name;
#define ELAINE_TEXTURE2D_ARRAY(Group, Slot, Name) layout(set = ELAINE_GROUP_##Group, binding = ELAINE_SLOT_##Slot) uniform sampler2DArray Name;
#define ELAINE_TEXTURE_CUBE(Group, Slot, Name) layout(set = ELAINE_GROUP_##Group, binding = ELAINE_SLOT_##Slot) uniform samplerCube Name;
#define ELAINE_READONLY_BUFFER(Group, Slot, Name) layout(std430, set = ELAINE_GROUP_##Group, binding = ELAINE_SLOT_##Slot) readonly buffer Name
#endif
