# Control-Flow Graph Liveness Sample
function branchyTest(n) {
  bb0:
    a = MOV 10
    b = MOV 20
    t1 = CMP a, n
    CJMP t1, bb1, bb2
  bb1:
    c = ADD a, b
    JMP bb3
  bb2:
    c = SUB b, a
    JMP bb3
  bb3:
    RET c
}
