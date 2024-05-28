use std::fmt::Display;

use enum_iterator::Sequence;

#[allow(non_camel_case_types)]
pub enum EepromType {
    Atmel_AT28C64,
    Microchip_24LCxx,
}

impl EepromType {
    pub fn get_id(&self) -> u8 {
        match *self {
            Self::Atmel_AT28C64 => 0,
            Self::Microchip_24LCxx => 1,
        }
    }
}

impl Display for EepromType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match *self {
            Self::Atmel_AT28C64 => f.write_str("Atmel AT28C64"),
            Self::Microchip_24LCxx => f.write_str("Microchip 24LCxx series"),
        }
    }
}

#[allow(non_camel_case_types)]
#[derive(Sequence, Clone, Copy)]
pub enum EepromVariant {
    Atmel_AT28C64,
    Microchip_24LC32,
}

impl EepromVariant {
    pub fn get_type(&self) -> EepromType {
        match *self {
            Self::Atmel_AT28C64 => EepromType::Atmel_AT28C64,
            Self::Microchip_24LC32 => EepromType::Microchip_24LCxx,
        }
    }

    pub fn get_capacity(&self) -> usize {
        match *self {
            Self::Atmel_AT28C64 => 8192,
            Self::Microchip_24LC32 => 4096,
        }
    }
}

impl Display for EepromVariant {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match *self {
            Self::Atmel_AT28C64 => f.write_str("Atmel AT28C64"),
            Self::Microchip_24LC32 => f.write_str("Microchip 24LC32"),
        }
    }
}
