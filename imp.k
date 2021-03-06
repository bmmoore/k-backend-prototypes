module IMP-SYNTAX

  syntax AExp  ::= Int | Id
                 | AExp "/" AExp              [left, strict]
                 > AExp "+" AExp              [left, strict]
                 | "(" AExp ")"               [bracket]
  syntax BExp  ::= Bool
                 | AExp "<=" AExp             [seqstrict]
                 | "!" BExp                   [strict]
                 > BExp "&&" BExp             [left, strict(1)]
                 | "(" BExp ")"               [bracket]
  syntax Block ::= "{" "}"
                 | "{" Stmt "}"
  syntax Stmt  ::= Block
                 | Id "=" AExp ";"            [strict(2)]
                 | "if" "(" BExp ")"
                   Block "else" Block         [strict(1)]
                 | "while" "(" BExp ")" Block
                 > Stmt Stmt                  [left]

  syntax Pgm ::= "int" Ids ";" Stmt
  syntax Ids ::= List{Id,","}
endmodule

module IMP
  imports IMP-SYNTAX

  syntax KResult ::= Int | Bool

  configuration <T color="yellow">
                  <k color="green"> $PGM:Pgm </k>
                  <state color="red"> .Map </state>
                  <exit exit="exit">0</exit>
                </T>

  rule <k> X:Id => I ...</k> <state>... X |-> I ...</state>

  rule I1 / I2 => I1 /Int I2  when I2 =/=Int 0
  rule I1 + I2 => I1 +Int I2

  rule I1 <= I2 => I1 <=Int I2
  rule ! T => notBool T
  rule true && B => B
  rule false && _ => false

  rule {} => .   [structural]
  rule {S} => S  [structural]

  rule <k> X = I:Int; => . ...</k> <state>... X |-> (_ => I) ...</state>

  rule S1:Stmt S2:Stmt => S1 ~> S2  [structural]

  rule if (true)  S else _ => S
  rule if (false) _ else S => S

  rule while (B) S => if (B) {S while (B) S} else {}  [structural]

  rule <k> int (X,Xs => Xs);_ </k> <state> Rho:Map (.Map => X|->0) </state>
    when notBool (X in keys(Rho))
  rule int .Ids; S => S  [structural]
endmodule
