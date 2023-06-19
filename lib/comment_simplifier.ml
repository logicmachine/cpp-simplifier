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

module Lexer : sig
  type err =
    | Eof_in_comment
    | Eof_in_string
    | New_line_in_string
    | No_code_after_multiline_block_comment

  exception Error of err * Lexing.position

  val line_start : (char -> unit) -> Lexing.lexbuf -> unit
  val pp_err : Format.formatter -> err -> unit
  val show_err : err -> string
end =
  Lexer

type input = Stdin | String | File of string
type position = { file : input; line : int; column : int; seek_pos : int }

let lex ?(print_char = print_char) file lexbuf =
  try Result.ok @@ Lexer.line_start print_char lexbuf
  with Lexer.Error (reason, pos) ->
    Result.error
      ( reason,
        {
          file;
          line = pos.pos_lnum;
          column = pos.pos_cnum - pos.pos_bol;
          seek_pos = pos.pos_bol;
        } )

let from_stdin () = lex Stdin @@ Lexing.from_channel stdin
let from_string str = lex String @@ Lexing.from_string str

let from_file ?tmp_file file =
  let print_char, close_out =
    match tmp_file with
    | None -> (print_char, fun () -> ())
    | Some tmp ->
        let channel = Stdlib.open_out tmp in
        (output_char channel, fun () -> close_out channel)
  in
  let channel = Stdlib.open_in file in
  Fun.protect
    (fun () -> lex ~print_char (File file) @@ Lexing.from_channel channel)
    ~finally:(fun () ->
      close_out ();
      close_in channel)

let format_err reason =
  Str.(
    reason |> Lexer.show_err
    |> global_replace (regexp "Lexer\\.") ""
    |> global_replace (regexp "_") " "
    |> String.lowercase_ascii)

let format_err reason pos =
  let file =
    match pos.file with
    | File file -> file
    | Stdin -> "stdin"
    | String -> "string"
  in
  Format.sprintf "%s:%d:%d: error: %s" file pos.line pos.column
    (format_err reason)

let format_err reason pos =
  let str = format_err reason pos in
  match pos.file with
  | Stdin | String -> str
  | File file ->
      let channel = open_in file in
      let line =
        Fun.protect
          (fun () ->
            seek_in channel pos.seek_pos;
            input_line channel)
          ~finally:(fun () -> close_in channel)
      in
      let caret = Bytes.make pos.column ' ' in
      Bytes.set caret (pos.column - 1) '^';
      String.concat "\n" [ str; line; Bytes.to_string caret ]
