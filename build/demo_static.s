# regalloc output
# k = 4
# cost model: static
# rounds: 2
# spilled: b -> S0
# moves removed by coalescing: 0
# registers used: 4
# register map:
#   a -> R2
#   c -> R3
#   d -> R1
#   e -> R0
#   m -> R3
#   g -> R0
#   i -> R3
#   j -> R0
#   b_t9 -> R0
#   b_t10 -> R0
#   b_t11 -> R0
#   b_t12 -> R0
label B0:
  R2 = 5
  R0 = 0
  spill R0 -> S0
  R3 = 6
  R1 = 1
  R0 = 0
  jmp H
label H:
  R0 = R0 + R1
  R0 = R0 + R3
  R0 = R0 + R1
  R0 = R0 + R1
  R3 = R3 - R1
  br R3, H, afterH
label afterH:
  print R0
  R3 = 3000
  jmp L2
label L2:
  reload R0 <- S0
  R0 = R0 + R1
  spill R0 -> S0
  R3 = R3 - R1
  br R3, L2, afterL2
label afterL2:
  reload R0 <- S0
  print R0
  R0 = R1 - R1
  br R0, coldinit, done
label coldinit:
  R3 = 3
  jmp outer
label done:
  ret R1
label outer:
  R0 = 4
  jmp inner
label inner:
  R2 = R2 + R1
  R2 = R2 + R1
  R0 = R0 - R1
  br R0, inner, outerEnd
label outerEnd:
  R3 = R3 - R1
  br R3, outer, coldEnd
label coldEnd:
  print R2
  jmp done
