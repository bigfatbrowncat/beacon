#!/usr/bin/env python

import argparse
import os
import subprocess
import sys

def main():
  parser = argparse.ArgumentParser(description='Convert a data file to an object file')
  parser.add_argument('--input', type=str, required=True)
  parser.add_argument('--output', type=str, required=True)
  parser.add_argument('--symbol_name', type=str, required=True)

  args = parser.parse_args()

  input_path = os.path.abspath(args.input)
  output_path = os.path.abspath(args.output)

  with open(output_path, "w") as f:
    f.write("\t.global _" + args.symbol_name + "_begin\n")
    f.write("\t.global _" + args.symbol_name + "_end\n")
    f.write("_" + args.symbol_name + "_begin:\n")
    f.write('\t.incbin "' + input_path + '"\n')
    f.write("_" + args.symbol_name + "_end:\n")

  print("Generated " + output_path)


if __name__ == '__main__':
  sys.exit(main())


#    .global _icudtl_dat_begin
#    .global _icudtl_dat_end
#_icudtl_dat_begin:
#    .incbin "somefile.gen.out"
#_icudtl_dat_end: