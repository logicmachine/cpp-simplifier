let stdio_run () =
  let lexbuf = Lexing.from_channel stdin in
  Lexer.start print_char lexbuf

