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

open Shexp_process
module List = Stdlib.List

let ( let* ) x f = bind ~f x
let list_files dir = FileUtil.find Is_file dir (Fun.flip Stdlib.List.cons) []

let comment_simplify file =
  with_temp_file ~prefix:"c-tree-carve-" ~suffix:".txt" (fun tmp_file ->
      Comment_simplifier.from_file ~tmp_file file;
      FileUtil.cp [ tmp_file ] file;
      return ())

let fixup output =
  Str.(
    global_replace
      (regexp "clang-tree-carve\\(\\.exe\\)?")
      "c-tree-carve" output)

let prog =
  let args = Array.to_list Sys.argv in
  let* exit_code, output =
    run_exit_code "clang-tree-carve.exe" (Stdlib.List.tl args)
    |> capture [ Std_io.Stdout; Std_io.Stderr ]
  in
  if exit_code <> 0 then
    let* () = eprint @@ fixup output in
    return 1
  else if FilePath.is_valid output then
    let* () =
      Shexp_process.List.iter ~f:comment_simplify @@ list_files output
    in
    let* () = echo ~n:() output in
    return 0
  else
    let* () = echo ~n:() (fixup output) in
    return 0

let () = exit @@ eval ~context:(Context.create ()) prog
