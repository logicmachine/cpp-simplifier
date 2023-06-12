module Cs = Comment_simplifier
module L = Cs.Lexer
;;

let ignor =
  "//-"
;;

let esc_nl =
  "\\\n"
;;

let line_start = 
  [ ignor ; "line_start" ]
;;

let kept_line =
  [ esc_nl; "\n"; "//"; "/*"; "\""; "kept_line" ]
;;

let kept_block_comment =
  [ "\n" ^ ignor; "\n"; "*/"; "kept_block_comment" ]
;;

let kept_str =
  [ esc_nl; "\\\\"; "\\\""; "\""; "\n"; "kept_str" ]
;;

let ignored_line =
  [ esc_nl ^ ignor; esc_nl; "\n"; "/*"; "//"; "ignored_line" ]
;;

let commented_line =
  [ esc_nl ^ ignor; esc_nl; "\n"; "commented_line" ]
;;

let ignored_block_comment =
  [ esc_nl ^ ignor; esc_nl; "\n"; "*/"; ignor; "ignored_block_comment" ]
;;

let empty_line =
  [ esc_nl; "//"; "\n"; "/*"; "empty_line" ]
;;

let test_string str =
  try
    Cs.from_string str;
  with L.Error No_code_after_multi_line_block_comment ->
      print_string "No_code_after_multi_line_block_comment"
     | L.Error Eof_in_comment ->
      print_string "Eof_in_comment"
     | L.Error Eof_in_string ->
       print_string "Eof_in_string"
     | L.Error Newline_in_string ->
      print_string "Newline_in_string"
;;

let test_all tests =
  List.iter (fun x ->
      print_string x;
      print_string "\n-->\n";
      test_string x;
      print_string "\n====\n")
    tests
;;

let test_with prefix tests =
  test_all @@ List.map (fun x -> String.concat " " (prefix @ [ x ])) tests
;;

let%expect_test "line_start -> _" =
  test_all line_start;
  [%expect {|
    //-
    -->

    ====
    line_start
    -->
    line_start
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> _" =
  test_with [ "line_start" ] kept_line;
  [%expect {|
    line_start \

    -->
    line_start \

    ====
    line_start

    -->
    line_start

    ====
    line_start //
    -->
    line_start //
    ====
    line_start /*
    -->
    line_start /*Eof_in_comment
    ====
    line_start "
    -->
    line_start "Eof_in_string
    ====
    line_start kept_line
    -->
    line_start kept_line
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> kept_line" =
  test_with [ "line_start" ; esc_nl ] kept_line;
  [%expect {|
    line_start \
     \

    -->
    line_start \
     \

    ====
    line_start \


    -->
    line_start \


    ====
    line_start \
     //
    -->
    line_start \
     //
    ====
    line_start \
     /*
    -->
    line_start \
     /*Eof_in_comment
    ====
    line_start \
     "
    -->
    line_start \
     "Eof_in_string
    ====
    line_start \
     kept_line
    -->
    line_start \
     kept_line
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> line_start" =
  test_with [ "line_start" ; "\n" ] line_start;
  [%expect {|
    line_start
     //-
    -->
    line_start
     //-
    ====
    line_start
     line_start
    -->
    line_start
     line_start
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> commented_line" =
  test_with [ "line_start" ; "//" ] commented_line;
  [%expect {|
    line_start // \
    //-
    -->
    line_start // \

    ====
    line_start // \

    -->
    line_start // \

    ====
    line_start //

    -->
    line_start //

    ====
    line_start // commented_line
    -->
    line_start // commented_line
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> commented_line" =
  test_with [ "line_start" ; "//"; esc_nl ^ ignor ] commented_line;
  [%expect {|
    line_start // \
    //- \
    //-
    -->
    line_start // \
     \

    ====
    line_start // \
    //- \

    -->
    line_start // \
     \

    ====
    line_start // \
    //-

    -->
    line_start // \


    ====
    line_start // \
    //- commented_line
    -->
    line_start // \
     commented_line
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> commented_line -> line_start" =
  test_with [ "line_start" ; "//"; "\n" ] line_start;
  [%expect {|
    line_start //
     //-
    -->
    line_start //
     //-
    ====
    line_start //
     line_start
    -->
    line_start //
     line_start
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> commented_line -> commented_line" =
  test_with [ "line_start" ; "//"; "commented_line" ] commented_line;
  [%expect {|
    line_start // commented_line \
    //-
    -->
    line_start // commented_line \

    ====
    line_start // commented_line \

    -->
    line_start // commented_line \

    ====
    line_start // commented_line

    -->
    line_start // commented_line

    ====
    line_start // commented_line commented_line
    -->
    line_start // commented_line commented_line
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> kept_block_comment" =
  test_with [ "line_start" ; "/*" ] kept_block_comment;
  [%expect {|
    line_start /*
    //-
    -->
    line_start /*
    Eof_in_comment
    ====
    line_start /*

    -->
    line_start /*
    Eof_in_comment
    ====
    line_start /* */
    -->
    line_start /* */
    ====
    line_start /* kept_block_comment
    -->
    line_start /* kept_block_commentEof_in_comment
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> kept_block_comment -> kept_block_comment" =
  test_with [ "line_start" ; "/*" ; "kept_block_comment" ] kept_block_comment;
  [%expect {|
    line_start /* kept_block_comment
    //-
    -->
    line_start /* kept_block_comment
    Eof_in_comment
    ====
    line_start /* kept_block_comment

    -->
    line_start /* kept_block_comment
    Eof_in_comment
    ====
    line_start /* kept_block_comment */
    -->
    line_start /* kept_block_comment */
    ====
    line_start /* kept_block_comment kept_block_comment
    -->
    line_start /* kept_block_comment kept_block_commentEof_in_comment
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> kept_block_comment -> kept_line" =
  test_with [ "line_start" ; "/*"; "*/" ] kept_line;
  [%expect {|
    line_start /* */ \

    -->
    line_start /* */ \

    ====
    line_start /* */

    -->
    line_start /* */

    ====
    line_start /* */ //
    -->
    line_start /* */ //
    ====
    line_start /* */ /*
    -->
    line_start /* */ /*Eof_in_comment
    ====
    line_start /* */ "
    -->
    line_start /* */ "Eof_in_string
    ====
    line_start /* */ kept_line
    -->
    line_start /* */ kept_line
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> kept_string" =
  test_with [ "line_start" ; "\"" ] kept_str;
  [%expect {|
    line_start " \

    -->
    line_start " \
    Eof_in_string
    ====
    line_start " \\
    -->
    line_start " \\Eof_in_string
    ====
    line_start " \"
    -->
    line_start " \"Eof_in_string
    ====
    line_start " "
    -->
    line_start " "
    ====
    line_start "

    -->
    line_start " Newline_in_string
    ====
    line_start " kept_str
    -->
    line_start " kept_strEof_in_string
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> kept_string -> kept_line" =
  test_with [ "line_start" ; "\""; "\"" ] kept_line;
  [%expect {|
    line_start " " \

    -->
    line_start " " \

    ====
    line_start " "

    -->
    line_start " "

    ====
    line_start " " //
    -->
    line_start " " //
    ====
    line_start " " /*
    -->
    line_start " " /*Eof_in_comment
    ====
    line_start " " "
    -->
    line_start " " "Eof_in_string
    ====
    line_start " " kept_line
    -->
    line_start " " kept_line
    ==== |}]
;;

let%expect_test "line_start -> kept_line -> kept_line" =
  test_with [ "line_start" ; "kept_line" ] kept_line;
  [%expect {|
    line_start kept_line \

    -->
    line_start kept_line \

    ====
    line_start kept_line

    -->
    line_start kept_line

    ====
    line_start kept_line //
    -->
    line_start kept_line //
    ====
    line_start kept_line /*
    -->
    line_start kept_line /*Eof_in_comment
    ====
    line_start kept_line "
    -->
    line_start kept_line "Eof_in_string
    ====
    line_start kept_line kept_line
    -->
    line_start kept_line kept_line
    ==== |}]
;;

let%expect_test "line_start -> ignored_line" =
  test_with [ ignor ] ignored_line;
  [%expect {|
    //- \
    //-
    -->
    //- \

    ====
    //- \

    -->
    //- \

    ====
    //-

    -->


    ====
    //- /*
    -->
    Eof_in_comment
    ====
    //- //
    -->
     //
    ====
    //- ignored_line
    -->
    //- ignored_line
    ==== |}]
;;

let%expect_test "line_start -> ignored_line -> commented_line" =
  test_with [ ignor ; "//" ] commented_line;
  [%expect {|
    //- // \
    //-
    -->
     // \

    ====
    //- // \

    -->
     // \

    ====
    //- //

    -->
     //

    ====
    //- // commented_line
    -->
     // commented_line
    ==== |}]
;;

let%expect_test "line_start -> ignored_line -> line_start" =
  test_with [ ignor ; "\n" ] line_start;
  [%expect {|
    //-
     //-
    -->

     //-
    ====
    //-
     line_start
    -->

     line_start
    ==== |}]
;;

let%expect_test "line_start -> ignored_line -> ignored_block_comment" =
  test_with [ ignor ; "/*" ] ignored_block_comment;
  [%expect {|
    //- /* \
    //-
    -->
    Eof_in_comment
    ====
    //- /* \

    -->
    Eof_in_comment
    ====
    //- /*

    -->
    Eof_in_comment
    ====
    //- /* */
    -->
     /* */
    ====
    //- /* //-
    -->
    Eof_in_comment
    ====
    //- /* ignored_block_comment
    -->
    Eof_in_comment
    ==== |}]
;;

let%expect_test "line_start -> ignored_line -> ignored_block_comment -> ignored_block_comment" =
  test_with [ ignor ; "/*" ; esc_nl ^ ignor ] ignored_block_comment;
  [%expect {|
    //- /* \
    //- \
    //-
    -->
    Eof_in_comment
    ====
    //- /* \
    //- \

    -->
    Eof_in_comment
    ====
    //- /* \
    //-

    -->
    Eof_in_comment
    ====
    //- /* \
    //- */
    -->
     /* \
     */
    ====
    //- /* \
    //- //-
    -->
    Eof_in_comment
    ====
    //- /* \
    //- ignored_block_comment
    -->
    Eof_in_comment
    ==== |}]
;;

let%expect_test "line_start -> ignored_line -> ignored_block_comment -> ignored_line" =
  test_with [ ignor ; "/*" ; "*/" ] ignored_line;
  [%expect {|
    //- /* */ \
    //-
    -->
    //- /* */ \

    ====
    //- /* */ \

    -->
    //- /* */ \

    ====
    //- /* */

    -->
     /* */

    ====
    //- /* */ /*
    -->
    Eof_in_comment
    ====
    //- /* */ //
    -->
     /* */ //
    ====
    //- /* */ ignored_line
    -->
    //- /* */ ignored_line
    ==== |}]
;;

let%expect_test "line_start -> ignored_line -> ignored_block_comment -> ignored_block_comment -> empty_line" =
  test_with [ ignor ; "/*" ; "\n*/" ] empty_line;
  [%expect {|
    //- /*
    */ \

    -->
     /*
    */ \

    ====
    //- /*
    */ //
    -->
     /*
    */ //
    ====
    //- /*
    */

    -->
     /*
    */

    ====
    //- /*
    */ /*
    -->
     /*
    */ /*Eof_in_comment
    ====
    //- /*
    */ empty_line
    -->
     /*
    */ No_code_after_multi_line_block_comment
    ==== |}]
;;

