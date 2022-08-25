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
assert 17 '1-8/(2*2)+3*6'

assert 10 '-10+20'
assert 10 '- -10'
assert 10 '- - +10'
assert 48 '------12*+++++----++++++++++4'

assert 0 '0==1'
assert 1 '42==42'
assert 1 '0!=1'
assert 0 '42!=42'
assert 1 '0<1'
assert 0 '1<1'
assert 0 '2<1'
assert 1 '0<=1'
assert 1 '1<=1'
assert 0 '2<=1'
assert 1 '1>0'
assert 0 '1>1'
assert 0 '1>2'
assert 1 '1>=0'
assert 1 '1>=1'
assert 0 '1>=2'
assert 1 '5==2+3'
assert 0 '6==4+3'
assert 1 '0*9+5*2==4+4*(6/3)-2'
#if success to pass, print OK!
echo OK!
