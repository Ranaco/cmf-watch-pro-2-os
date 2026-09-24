fn main() -> std::io::Result<()> {
    let address = std::env::args()
        .nth(1)
        .unwrap_or_else(|| cmf_watch_host::server::DEFAULT_ADDRESS.to_owned());
    cmf_watch_host::server::run(&address)
}
