mod error;
mod serial_io;
mod variant;

use std::{env, process};

use dialoguer::Select;
use error::Error;
use indicatif::ProgressIterator;
use itertools::Itertools;
use serial_io::Session;
use variant::EepromVariant;

enum Action {
    ReadRom,
    WriteRom,
}

struct Config {
    action: Action,
    file: String,
    serial_port: Option<String>,
    variant: Option<EepromVariant>,
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

        // TODO: Parse serial port and variant (clap?)
        Ok(Config {
            action,
            file,
            serial_port: None,
            variant: None,
        })
    }
}

fn verify_rom(session: &mut Session, expected_data: &Vec<u8>) -> Result<(), Error> {
    println!("Verifying...");
    for (page, data) in expected_data.chunks(256).progress().enumerate() {
        let page_data = session.read_page(page as u8)?;

        if page_data[0..data.len()] != *data {
            return Err(Error::VerifyFailed);
        }
    }
    Ok(())
}

fn read_rom(
    port_name: &str,
    variant: EepromVariant,
    file_name: String,
    eeprom_size: usize,
) -> Result<(), Error> {
    let rom_data = {
        let mut session = Session::open(port_name, variant)?;

        println!("Reading EEPROM...");
        let mut rom_data: Vec<u8> = Vec::with_capacity(eeprom_size);
        for page in (0..eeprom_size / 256).progress() {
            let page_data = session.read_page(page as u8)?;
            rom_data.extend(page_data.iter());
        }
        verify_rom(&mut session, &rom_data)?;
        rom_data
    };

    println!("Saving output to {file_name}...");
    std::fs::write(file_name, rom_data)?;

    Ok(())
}

fn write_rom(
    port_name: &str,
    variant: EepromVariant,
    file_name: String,
    eeprom_size: usize,
) -> Result<(), Error> {
    println!("Reading {file_name}...");
    let rom_data = std::fs::read(file_name)?;
    if rom_data.len() > eeprom_size {
        return Err(Error::RomTooBig);
    }

    {
        let mut session = Session::open(port_name, variant)?;

        println!("Writing EEPROM...");
        for (page, data) in rom_data.chunks(256).progress().enumerate() {
            let buffer: [u8; 256] = match data.try_into() {
                Ok(buf) => buf,
                Err(_) => {
                    // Pad the remaining bytes with 0
                    let mut buf = [0; 256];
                    buf[0..data.len()].copy_from_slice(data);
                    buf
                }
            };

            session.write_page(page as u8, &buffer)?;
        }

        verify_rom(&mut session, &rom_data)?;
    }

    println!("Write successful");
    Ok(())
}

pub fn choose_serial_port() -> String {
    // Find Teensy
    let ports = serial_io::find_serial_ports();

    let port = match ports.len() {
        0 => {
            eprintln!("No Teensy found. Make sure the EEPROM programmer is connected.");
            process::exit(1);
        }
        1 => {
            let port = &ports[0];
            println!("Using serial port: {}", port.port_name);
            port
        }
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

fn choose_variant() -> EepromVariant {
    let variants = enum_iterator::all::<EepromVariant>().collect_vec();

    let selection = Select::new()
        .with_prompt("Choose the type of EEPROM")
        .items(&variants)
        .interact()
        .unwrap();
    let selection = variants[selection];

    println!("EEPROM type: {} ({})", selection, selection.get_type());
    selection
}

fn run(config: Config) -> Result<(), Error> {
    let Config {
        action,
        file,
        serial_port,
        variant,
    } = config;

    let port_name = serial_port.unwrap_or_else(choose_serial_port);
    let variant = variant.unwrap_or_else(choose_variant);

    let eeprom_size = variant.get_capacity();
    match action {
        Action::ReadRom => read_rom(&port_name, variant, file, eeprom_size),
        Action::WriteRom => write_rom(&port_name, variant, file, eeprom_size),
    }
}

pub fn main() {
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
