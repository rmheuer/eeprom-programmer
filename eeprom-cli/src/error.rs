use thiserror::Error;

#[derive(Error, Debug)]
pub enum Error {
    #[error("ROM file too large to fit on EEPROM")]
    RomTooBig,

    #[error("programmer did not acknowledge write")]
    NoWriteAck,

    #[error("verification failed")]
    VerifyFailed,

    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),

    #[error("Serial error: {0}")]
    Serial(#[from] serialport::Error),
}
