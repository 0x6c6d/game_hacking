## driver_3

The goal is to set thread priorities. The Windows API function "SetPriorityClass()" only allows a small set of priorities to bet set directly. This driver would circumvent these limitations and allow setting a thread’s priority to any number, regardless of its process priority class.