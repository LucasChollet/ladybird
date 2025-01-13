use std::{env, path::PathBuf};
use zngur::Zngur;

fn main() {
    println!("cargo::rerun-if-changed=main.zng");

    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());

    let generated_header_path = PathBuf::from(env::var("LADYBIRD_RUST_HEADER_OUTPUT").unwrap());


    Zngur::from_zng_file(crate_dir.join("main.zng"))
        .with_rs_file(out_dir.join("generated.rs"))
        .with_h_file(generated_header_path)
        .generate();
}