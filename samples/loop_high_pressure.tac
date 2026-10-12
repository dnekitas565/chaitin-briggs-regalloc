# Nine values are live at once inside the loop (> k = 4).
# Expected: prints 105, returns 105.  Block executions: entry 1, loop 5, exit 1.
a = 1
b = 2
c = 3
d = 4
e = 5
f = 6
one = 1
n = 5
s = 0
label loop:
  s = s + a
  s = s + b
  s = s + c
  s = s + d
  s = s + e
  s = s + f
  n = n - one
  br n, loop, exit
label exit:
  print s
  ret s
