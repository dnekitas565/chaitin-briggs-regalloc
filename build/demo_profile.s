# regalloc output
# k = 4
# cost model: profile
# rounds: 2
# spilled: a -> S0
# moves removed by coalescing: 0
# registers used: 4
# register map:
#   b -> R1
#   c -> R3
#   d -> R2
#   e -> R0
#   m -> R0
#   g -> R0
#   i -> R3
#   j -> R1
#   a_t9 -> R0
#   a_t10 -> R0
#   a_t11 -> R0
#   a_t12 -> R0
#   a_t13 -> R0
#   a_t14 -> R0
label B0:
  R0 = 5
  spill R0 -> S0
  R1 = 0
  R3 = 6
  R2 = 1
  R0 = 0
  jmp H
label H:
  R0 = R0 + R2
  R0 = R0 + R3
  R0 = R0 + R2
  R0 = R0 + R2
  R3 = R3 - R2
  br R3, H, afterH
label afterH:
  print R0
  R0 = 3000
  jmp L2
label L2:
  R1 = R1 + R2
  R0 = R0 - R2
  br R0, L2, afterL2
label afterL2:
  print R1
  R0 = R2 - R2
  br R0, coldinit, done
label coldinit:
  R3 = 3
  jmp outer
label done:
  ret R2
label outer:
  R1 = 4
  jmp inner
label inner:
  reload R0 <- S0
  R0 = R0 + R2
  spill R0 -> S0
  reload R0 <- S0
  R0 = R0 + R2
  spill R0 -> S0
  R1 = R1 - R2
  br R1, inner, outerEnd
label outerEnd:
  R3 = R3 - R2
  br R3, outer, coldEnd
label coldEnd:
  reload R0 <- S0
  print R0
  jmp done
