assert() {
  expected="$1"
  input="$2"

  ./bcc "$input" > tmp.s
  #riscv64-unknown-linux-gnu-gcc -static -o tmp tmp.s
  riscv64-unknown-linux-gnu-gcc -static tmp.s -o tmp
  qemu-riscv64 tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}


assert 0 0
assert 41 41
assert 56 '12+55-11'
assert 56 '12 + 55 - 11'

#if success to pass, print OK!
echo OK!
