let stdio_run () =
  let lexbuf = Lexing.from_channel stdin in
  Lexer.line_start print_char lexbuf

