import argparse
import sys

def bin2header(data, var_name='var'):
    out = []
    out.append('#include <stdint.h>')
    out.append('')
    out.append('uint8_t {var_name}[] = {{'.format(var_name=var_name))
    l = [ data[i:i+12] for i in range(0, len(data), 12) ]
    for i, x in enumerate(l):
        line = ', '.join([ '0x{val:02x}'.format(val=ord(c)) for c in x ])
        out.append('  {line}{end_comma}'.format(line=line, end_comma=',' if i<len(l)-1 else ''))
    out.append('};')
    out.append('uint32_t {var_name}_size = {data_len};'.format(var_name=var_name, data_len=len(data)))
    return '\n'.join(out)

def main():
    parser = argparse.ArgumentParser(description='Generate binary header output')
    parser.add_argument('-i', '--input', required=True , help='Input file')
    parser.add_argument('-o', '--output', required=True , help='Output file')
    parser.add_argument('-s', '--symbol_name', required=True , help='Variable name to use in file')

    args = parser.parse_args()
    if not args:
        return 1

    with open(args.input, 'rb') as f:
        data = f.read()

    out = bin2header(data, args.symbol_name)
    with open(args.output, 'w') as f:
        f.write(out)

    return 0

if __name__ == '__main__':
    sys.exit(main())