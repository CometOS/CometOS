#ifndef CFS_OPCODES_H_
#define CFS_OPCODES_H_

enum {
    CFS_OP_READ,
    CFS_OP_WRITE,
    CFS_OP_REMOVE,
    CFS_OP_READDIR,
};
typedef uint8_t cfs_op_t;

#endif /* CFS_OPCODES_H_ */
