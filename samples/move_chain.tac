# Chains of copies that coalescing should remove.  Expected: prints 10 then 10, returns 10.
a = 5
b = a
c = b
d = c
e = d
f = e + a
print f
g = f
h = g
print h
ret h
