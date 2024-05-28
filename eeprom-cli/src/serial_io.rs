use std::{process, time::Duration};

use itertools::Itertools;
use serialport::{SerialPort, SerialPortInfo, SerialPortType};

use crate::{error::Error, variant::EepromVariant};

pub fn find_serial_ports() -> Vec<SerialPortInfo> {
    serialport::available_ports()
        .unwrap_or_else(|e| {
            eprintln!("Failed to enumerate serial ports: {e}");
            process::exit(1);
        })
        .into_iter()
        .filter(|p| match &p.port_type {
            SerialPortType::UsbPort(a) => match &a.manufacturer {
                Some(s) => s == "Teensyduino",
                None => false,
            },
            _ => false,
        })
        .collect_vec()
}

pub struct Session {
    port: Box<dyn SerialPort>,
}

impl Session {
    pub fn open(port_name: &str, variant: EepromVariant) -> Result<Self, serialport::Error> {
        let mut port = serialport::new(port_name, 9600)
            .timeout(Duration::from_secs(10))
            .open()?;

        let cmd = [b'd', variant.get_type().get_id()];
        port.write_all(&cmd)?;

        Ok(Session { port })
    }

    pub fn read_page(&mut self, page_addr: u8) -> serialport::Result<[u8; 256]> {
        let cmd = ['r' as u8, page_addr];
        self.port.write_all(&cmd)?;

        let mut buf = [0; 256];
        self.port.read_exact(&mut buf)?;

        Ok(buf)
    }

    pub fn write_page(&mut self, page_addr: u8, data: &[u8; 256]) -> Result<(), Error> {
        let cmd = ['w' as u8, page_addr];
        self.port.write_all(&cmd)?;
        self.port.write_all(data)?;

        let mut ack = [0; 1];
        self.port.read_exact(&mut ack)?;

        if ack[0] == 0xa5 {
            Ok(())
        } else {
            Err(Error::NoWriteAck)
        }
    }
}

impl Drop for Session {
    fn drop(&mut self) {
        let cmd = [b'q'];
        if let Err(e) = self.port.write_all(&cmd) {
            eprintln!("Failed to close session: {e}");
        }
    }
}
