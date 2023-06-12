module Lexer = Lexer

let stdio_run () =
  let lexbuf = Lexing.from_channel stdin in
  Lexer.line_start print_char lexbuf

let from_string str =
  let lexbuf = Lexing.from_string str in
  Lexer.line_start print_char lexbuf
