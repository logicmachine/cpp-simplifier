(***************************************************************************)
(*  Copyright (c) 2022 Dhruv Makwana                                       *)
(*  All rights reserved.                                                   *)
(*                                                                         *)
(*  This software was developed by the University of Cambridge Computer    *)
(*  Laboratory as part of the Rigorous Engineering of Mainstream Systems   *)
(*  (REMS) project. This project has been partly funded by an EPSRC        *)
(*  Doctoral Training studentship. This project has been partly funded by  *)
(*  Google. This project has received funding from the European Research   *)
(*  Council (ERC) under the European Union's Horizon 2020 research and     *)
(*  innovation programme (grant agreement No 789108, Advanced Grant        *)
(*  ELVER).                                                                *)
(*                                                                         *)
(*  BSD 2-Clause License                                                   *)
(*                                                                         *)
(*  Redistribution and use in source and binary forms, with or without     *)
(*  modification, are permitted provided that the following conditions     *)
(*  are met:                                                               *)
(*  1. Redistributions of source code must retain the above copyright      *)
(*     notice, this list of conditions and the following disclaimer.       *)
(*  2. Redistributions in binary form must reproduce the above copyright   *)
(*     notice, this list of conditions and the following disclaimer in     *)
(*     the documentation and/or other materials provided with the          *)
(*     distribution.                                                       *)
(*                                                                         *)
(*  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''     *)
(*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED      *)
(*  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A        *)
(*  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR    *)
(*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,           *)
(*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT       *)
(*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF       *)
(*  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND    *)
(*  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,     *)
(*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT     *)
(*  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF     *)
(*  SUCH DAMAGE.                                                           *)
(***************************************************************************)

{
open Lexing
;;

type err =
  (* TODO add file name *)
  | Eof_in_comment
  (* TODO add location *)
  | Eof_in_string
  (* TODO add location *)
  | Newline_in_string
  (* TODO add location information *)
  | No_code_after_multi_line_block_comment
;;

exception Error of err
;;

let rev_char_list str =
  let seq = String.to_seq str in
  Seq.fold_left (fun y x -> List.cons x y) [] seq
;;

let newline (cont : _ -> lexbuf -> 'a) ~(write : char -> unit) (lexbuf : lexbuf) : 'a =
  write (lexeme_char lexbuf 0);
  new_line lexbuf;
  cont write lexbuf
;;

let esc_newline cont ~write lexbuf =
  String.iter write "\\\n";
  new_line lexbuf;
  cont write lexbuf
;;

}

let space_tab = ' ' | '\t'
let ws = space_tab | '\\' space_tab
let esc_nl = '\\' '\n'
let ignor = "//-"

(* start of a line, may or may not be kept *)
rule line_start write = parse
  | ignor { ignored_line [] write lexbuf }
  | eof   { () }
  | _     { write (lexeme_char lexbuf 0);
            kept_line write lexbuf }

(* line kept so cannot run-over with block-comment *)
and kept_line write = parse
  | eof    { () }
  | esc_nl { esc_newline kept_line ~write lexbuf }
  | '\n'   { newline line_start ~write lexbuf }
  | "//"   { String.iter write (lexeme lexbuf);
             commented_line write lexbuf }
  | "/*"   { String.iter write (lexeme lexbuf);
             kept_block_comment write lexbuf;
             kept_line write lexbuf }
  | '"'    { write (lexeme_char lexbuf 0);
             kept_str write lexbuf;
             kept_line write lexbuf }
  | _      { write (lexeme_char lexbuf 0);
             kept_line write lexbuf }

and kept_block_comment write = parse
  | eof        { raise @@ Error Eof_in_comment }
  | '\n' ignor
  | '\n'       { newline kept_block_comment ~write lexbuf }
  | "*/"       { String.iter write (lexeme lexbuf) }
  | _          { write (lexeme_char lexbuf 0);
                 kept_block_comment write lexbuf }

and kept_str write = parse
  | eof      { raise @@ Error Eof_in_string }
  | esc_nl   { esc_newline kept_str ~write lexbuf }
  | '\\' '\\'
  | '\\' '"' { String.iter write (lexeme lexbuf);
               kept_str write lexbuf }
  | '"'      { write (lexeme_char lexbuf 0) }
  | '\n'     { raise @@ Error Newline_in_string }
  | _        { write (lexeme_char lexbuf 0);
               kept_str write lexbuf }

(* assume: - in ignored line,"//-" not written, `buf' seen so far *)
and ignored_line buf write = parse
  | eof    { List.iter write (List.rev buf) }
  | ws     { ignored_line (rev_char_list (lexeme lexbuf) @ buf) write lexbuf }
  | esc_nl ignor
  | esc_nl { String.iter write "//-";
             List.iter write (List.rev buf);
             esc_newline commented_line ~write lexbuf }
  | '\n' { List.iter write (List.rev buf);
           newline line_start ~write lexbuf }
  | "/*" { let buf = rev_char_list (lexeme lexbuf) @ buf in
           let (n, buf) = ignored_block_comment (1,buf) lexbuf in
           if n = 1  then
             ignored_line buf write lexbuf
           else (
             List.iter write (List.rev buf);
             empty_line write lexbuf
           )
         }
  | "//" { List.iter write (List.rev buf);
           String.iter write (lexeme lexbuf);
           commented_line write lexbuf }
  | _    { String.iter write "//-";
           List.iter write (List.rev buf);
           write (lexeme_char lexbuf 0);
           commented_line write lexbuf }

(* rest of line is line-commented out *)
and commented_line write = parse
  | eof    { () }
  | esc_nl ignor
  | esc_nl { esc_newline commented_line ~write lexbuf }
  | '\n'   { newline line_start ~write lexbuf }
  | _      { write (lexeme_char lexbuf 0);
             commented_line write lexbuf }

(* in a block comment, with //- delete-able *)
and ignored_block_comment lines_buf = parse
  | eof    { raise @@ Error Eof_in_comment }
  (* weird, yes, but \ removal takes place _before_ comment recognition *)
  | esc_nl ignor
  | esc_nl { let (lines, buf) = lines_buf in
             ignored_block_comment (lines, rev_char_list "\\\n" @ buf) lexbuf }
  | '\n'   { let (lines, buf) = lines_buf in
             newline
              (Fun.const (ignored_block_comment (lines+1, (lexeme_char lexbuf 0) :: buf)))
              ~write:(fun _ -> ()) lexbuf }
  | "*/"   { let (lines, buf) = lines_buf in (lines, (rev_char_list (lexeme lexbuf) @ buf)) }
  | ignor  { ignored_block_comment lines_buf lexbuf }
  | _      { let (lines, buf) = lines_buf in
             let buf = (lexeme_char lexbuf 0) :: buf in
             ignored_block_comment (lines, buf) lexbuf }

(* line is kept, but must be empty *)
and empty_line write = parse
  | eof    { () }
  | esc_nl { esc_newline empty_line ~write lexbuf }
  | "//"   { String.iter write (lexeme lexbuf);
             commented_line write lexbuf }
  | '\n'   { newline line_start ~write lexbuf }
  | "/*"   { String.iter write (lexeme lexbuf);
             kept_block_comment write lexbuf;
             empty_line write lexbuf }
  | ws     { String.iter write (lexeme lexbuf);
             empty_line write lexbuf }
  | _      { raise @@ Error No_code_after_multi_line_block_comment }

