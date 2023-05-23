{
open Lexing
;;
}


rule start write = parse
  | "//-" { in_ignored [] write lexbuf }
  | eof   { () }
  | _     { write (lexeme_char lexbuf 0);
            write_line write lexbuf }

and write_line write = parse
  | eof  { () }
  | '\n' { write '\n'; new_line lexbuf; start write lexbuf }
  | _    { write (lexeme_char lexbuf 0); write_line write lexbuf }

and in_ignored spaces write = parse
  | ' ' | '\t' { in_ignored ((lexeme_char lexbuf 0) :: spaces) write lexbuf }
  | "//"  { List.iter write (List.rev spaces);
            write (lexeme_char lexbuf 0);
            write (lexeme_char lexbuf 1);
            write_line write lexbuf }
  | _     { String.iter write "//-";
            List.iter write (List.rev spaces);
            write (lexeme_char lexbuf 0);
            write_line write lexbuf }
