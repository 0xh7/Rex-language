use rex::io
use rex::url
use rex::collections as col

fn main() {
    let base = "https://example.com/api"
    let part = "v1/search"
    let query_text = "rex language"

    mut params = col.map_new<str, str>()
    col.map_put(&mut params, "q", "rex language")
    col.map_put(&mut params, "page", "1")

    let joined = url.join(&base, &part)
    let full = url.with_query(&joined, &params)
    let encoded = url.encode_component(&query_text)

    match url.decode_component(&encoded) {
        Ok(decoded) => println(decoded),
        Err(e) => println("decode error: " + e),
    }

    match url.parse(&full) {
        Ok(parts) => {
            println(col.map_get(&parts, "scheme"))
            println(col.map_get(&parts, "host"))
            println(col.map_get(&parts, "path"))
            println(col.map_get(&parts, "query"))
        },
        Err(e) => println("parse error: " + e),
    }

    println(joined)
    println(full)
    println(encoded)
}
