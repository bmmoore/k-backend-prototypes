module SUM-SYNTAX
  syntax State ::= sum_pgm(Int)
                   loop_head(Int,Int)
                   result(Int)
endmodule

module SUM
  imports IMP-SYNTAX

  configuration <k color="green"> State </k>

  rule sum_pgm(N) => loop_head(N,0)
  rule loop_head(N,Sum) => loop_head(N -Int 1, Sum +Int N)
      requires N >Int 0
  rule loop_head(0,Sum) => result(Sum)
endmodule
