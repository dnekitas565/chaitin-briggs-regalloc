# Sample where static cost model and profile cost model disagree
function profileDemo() {
  bb0:
    loop_var = MOV 0
    JMP bb1
  bb1:
    cond = CMP loop_var, 1000
    CJMP cond, bb2, bb3
  bb2:
    loop_var = ADD loop_var, 1
    JMP bb1
  bb3:
    RET loop_var
}
