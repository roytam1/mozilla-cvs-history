function test() {
  ok(true, "pass ok");
  is(true, true, "pass is");
  isnot(false, true, "pass isnot");
  todo(false, "pass todo");
  todo_is(false, true, "pass todo_is");
  todo_isnot(true, true, "pass todo_isnot");

  var func = is;
  func(true, true, "pass indirect is");
}
