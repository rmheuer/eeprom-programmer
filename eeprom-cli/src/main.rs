use std::{env, process, time::Duration, u8};

use dialoguer::Select;
use indicatif::ProgressIterator;
use itertools::Itertools;
use serialport::{SerialPort, SerialPortType};
use thiserror::Error;

const EEPROM_SIZE: usize = 8192;
const PAGE_COUNT: usize = EEPROM_SIZE / 256;

#[derive(Error, Debug)]
enum Error {
    #[error("programmer did not acknowledge write")]
    NoWriteAck,

    #[error("verification failed")]
    VerifyFailed,

    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),

    #[error("Serial error: {0}")]
    Serial(#[from] serialport::Error),
}

fn choose_serial_port() -> String {
    // Find Teensy
    let ports = serialport::available_ports()
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
        .collect_vec();

    let port = match ports.len() {
        0 => {
            eprintln!("No Teensy found. Make sure the EEPROM programmer is connected.");
            process::exit(1);
        }
        1 => &ports[0],
        _ => {
            let selection = Select::new()
                .with_prompt(
                    "Multiple Teensy ports available. Choose which is the EEPROM programmer",
                )
                .items(&ports.iter().map(|p| &p.port_name).collect_vec())
                .interact()
                .unwrap();

            &ports[selection]
        }
    };

    port.port_name.clone()
}

fn read_page(port: &mut Box<dyn SerialPort>, page_addr: u8) -> serialport::Result<[u8; 256]> {
    let cmd = ['r' as u8, page_addr];
    port.write_all(&cmd)?;

    let mut buf = [0; 256];
    port.read_exact(&mut buf)?;

    Ok(buf)
}

fn write_page(port: &mut Box<dyn SerialPort>, page_addr: u8, data: [u8; 256]) -> Result<(), Error> {
    let cmd = ['w' as u8, page_addr];
    port.write_all(&cmd)?;
    port.write_all(&data)?;

    let mut ack = [0; 1];
    port.read_exact(&mut ack)?;

    if ack[0] == 0xa5 {
        Ok(())
    } else {
        Err(Error::NoWriteAck)
    }
}

enum Action {
    WriteRom,
    ReadRom,
}

struct Config {
    action: Action,
    file: String,
}

impl Config {
    fn build(args: &[String]) -> Result<Config, String> {
        if args.len() < 3 {
            return Err("not enough arguments".into());
        }

        let action = match args[1].as_str() {
            "w" | "write" => Action::WriteRom,
            "r" | "read" => Action::ReadRom,
            act => return Err(format!("invalid action: {act}")),
        };

        let file = args[2].clone();

        Ok(Config { action, file })
    }
}

fn needed_pages(len: usize) -> usize {
    (len - 1) / 256 + 1
}

fn verify_rom(port: &mut Box<dyn SerialPort>, expected_data: &Vec<u8>) -> Result<(), Error> {
    println!("Verifying...");
    for page in (0..needed_pages(expected_data.len())).progress() {
        let addr = page << 8;
        let known_data = &expected_data[addr..(addr + 256).min(expected_data.len())];
        let page_data = read_page(port, page as u8)?;

        if known_data.len() == page_data.len() {
            if known_data != page_data {
                return Err(Error::VerifyFailed);
            }
        } else {
            for i in 0..known_data.len() {
                if known_data[i] != page_data[i] {
                    return Err(Error::VerifyFailed);
                }
            }
        }
    }

    Ok(())
}

fn write_rom(file_name: String, mut port: Box<dyn SerialPort>) -> Result<(), Error> {
    println!("Reading {file_name}...");
    let rom_data = std::fs::read(file_name)?;

    println!("Writing EEPROM...");
    for page in (0..needed_pages(rom_data.len())).progress() {
        let addr = page << 8;

        // There's probably a better way to do this...
        let mut buffer = [0; 256];
        for i in 0..(rom_data.len() - addr).min(256) {
            buffer[i] = rom_data[addr + i];
        }

        write_page(&mut port, page as u8, buffer)?;
    }

    verify_rom(&mut port, &rom_data)?;
    println!("Write successful");

    Ok(())
}

fn read_rom(file_name: String, mut port: Box<dyn SerialPort>) -> Result<(), Error> {
    let mut rom_data: Vec<u8> = Vec::with_capacity(EEPROM_SIZE);

    println!("Reading EEPROM...");
    for page in (0..PAGE_COUNT).progress() {
        let page_data = read_page(&mut port, page as u8)?;
        rom_data.extend(page_data.iter());
    }

    verify_rom(&mut port, &rom_data)?;

    println!("Saving output to {file_name}...");
    std::fs::write(file_name, rom_data)?;

    Ok(())
}

fn run(config: Config) -> Result<(), Error> {
    let port_name = choose_serial_port();
    println!("Using serial port: {}", port_name);
    let port = serialport::new(port_name, 9600)
        .timeout(Duration::from_secs(10))
        .open()?;

    let Config { action, file } = config;
    match action {
        Action::WriteRom => write_rom(file, port),
        Action::ReadRom => read_rom(file, port),
    }
}

fn main() {
    let args = env::args().collect_vec();
    let config = Config::build(&args).unwrap_or_else(|err| {
        eprintln!("Problem parsing arguments: {err}");
        process::exit(1);
    });

    if let Err(e) = run(config) {
        eprintln!("Error: {e}");
        process::exit(1);
    }
}
