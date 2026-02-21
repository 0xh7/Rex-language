use rex::io
use rex::fmt
use rex::math

fn main() {
    println("Rex Console Calculator")
    println("type expressions like: 2+2, 14 / 2, 3.5*8")
    println("press Ctrl+C to quit")

    while true {
        print("op> ")
        match io.read_line() {
            Ok(line) => {
                match math.eval(&line) {
                    Ok(value) => {
                        println("= " + fmt.format(value))
                    },
                    Err(e) => {
                        println("error: " + e)
                    },
                }
            },
            Err(e) => {
                println("error: " + e)
                println("bye")
                break
            },
        }
    }
}
