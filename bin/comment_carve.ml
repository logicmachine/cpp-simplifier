let () = match Comment_simplifier.from_stdin () with
  | Result.Ok () -> ()
  | Result.Error (r, p) ->
    (print_string @@ Comment_simplifier.format_err r p;
     exit 1)

