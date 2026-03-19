use rex::io
use rex::process

fn main() {
    let run_cmd = "echo rex-process-run"
    let capture_cmd = "echo rex-process-capture"

    match process.run(&run_cmd) {
        Ok(code) => println("run exit: " + format(code)),
        Err(e) => println("run error: " + e),
    }

    match process.capture(&capture_cmd) {
        Ok(pair) => {
            let (code, output) = pair
            println("capture exit: " + format(code))
            println(output)
        },
        Err(e) => println("capture error: " + e),
    }
}
