
let project = new Project("kmake");

project.addIncludeDir("../../sources/libs");
project.addFile("../../sources/libs/quickjs/*.c");
project.addFile("main.c");

project.addDefine("environ=__environ");
project.addDefine("sighandler_t=__sighandler_t");

project.addDefine("WIN32_LEAN_AND_MEAN");
project.addDefine("_WIN32_WINNT=0x0602");

if (platform === 'linux') {
	project.addLib("m");
}

// Windows:
// C/C++ - Command Line:
// /experimental:c11atomics /std:c11
// Linker - Command Line:
// /subsystem:console

// QuickJS changes:
// quickjs-libc.c#85 (fixes "import * as os from 'os';" crash):
// #define USE_WORKER -> //#define USE_WORKER
// "quickjs.h#259" (fixes "Maximum call stack size exceeded" in minits):
// #define JS_DEFAULT_STACK_SIZE (256 * 1024) -> #define JS_DEFAULT_STACK_SIZE (8 * 1024 * 1024)

project.flatten();
return project;
