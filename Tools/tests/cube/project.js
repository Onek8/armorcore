
let flags = globalThis.flags;
// flags.with_hermes = true;
flags.with_eval = false;
// flags.with_g2 = true;
flags.with_iron = true;
// flags.with_zui = true;

let project = new Project("Test");
project.addSources("Sources");
project.addShaders("Shaders/*.glsl");
project.addAssets("Assets/*", { destination: "data/{name}" });

project.addDefine("arm_data_dir");

resolve(project);