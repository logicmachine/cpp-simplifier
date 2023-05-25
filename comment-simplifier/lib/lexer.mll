{
open Lexing
;;

(* TODO add file name *)
exception Eof_in_comment
;;

(* TODO add location *)
exception Only_one_line_block_comment
;;

(* TODO add location information *)
exception No_code_after_end_of_block_comment
;;
}

let ws = ' ' | '\t'

(* start of a line, may or may not be included *)
rule line_start write = parse
  | "//-" { ignored_line [] write lexbuf }
  | eof   { () }
  | _     { write (lexeme_char lexbuf 0);
            included_line write lexbuf }

(* line included so cannot run-over with block-comment *)
(* TODO what about \ at the end of a line? aargh *)
and included_line write = parse
  | eof   { () }
  | '\n'  { write '\n'; new_line lexbuf;
            line_start write lexbuf }
  | "/*"  { String.iter write "/*";
            one_line_block_comment write lexbuf;
            included_line write lexbuf }
  | _     { write (lexeme_char lexbuf 0);
            included_line write lexbuf }

and one_line_block_comment write = parse
  | eof  { raise Eof_in_comment }
  | '\n' { raise Only_one_line_block_comment }
  | "*/" { String.iter write "*/" }
  | _    { write (lexeme_char lexbuf 0);
           one_line_block_comment write lexbuf }

(* assume: - in ignored line, "//-" NOT written,
   only `spaces' or `comments' so far *)
and ignored_line spaces write = parse
  | ws   { ignored_line ((lexeme_char lexbuf 0) :: spaces) write lexbuf }
  | "/*" { List.iter write (List.rev spaces);
           String.iter write "/*";
           ignored_block_comment write lexbuf }
  | "//" { List.iter write (List.rev spaces);
           String.iter write "//";
           commented_line write lexbuf }
  | _    { String.iter write "//-";
           List.iter write (List.rev spaces);
           write (lexeme_char lexbuf 0);
           commented_line write lexbuf }

(* rest of line is line-commented out *)
and commented_line write = parse
  | eof   { () }
  | '\n'  { write '\n'; new_line lexbuf;
            line_start write lexbuf }
  | _     { write (lexeme_char lexbuf 0);
            commented_line write lexbuf }

(* in a block comment, with //- delete-able *)
and ignored_block_comment write = parse
  | eof   { raise Eof_in_comment }
  | '\n'  { write '\n'; new_line lexbuf;
            ignored_block_comment write lexbuf }
  | "*/"  { String.iter write "*/";
            empty_line write lexbuf }
  | "//-" { ignored_block_comment write lexbuf }
  | _     { write (lexeme_char lexbuf 0);
            ignored_block_comment write lexbuf }

(* line is included, but must be empty *)
and empty_line write = parse
  | eof  { () }
  | '\n' { write '\n'; new_line lexbuf;
           line_start write lexbuf }
  | "/*" { String.iter write "/*";
           one_line_block_comment write lexbuf;
           empty_line write lexbuf }
  | ws    { write (lexeme_char lexbuf 0);
            empty_line write lexbuf }
  | _     { raise No_code_after_end_of_block_comment }

