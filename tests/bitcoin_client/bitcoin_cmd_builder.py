import enum
import logging
import struct
from typing import List, Tuple, Union, Iterator, cast

from bitcoin_client.transaction import Transaction
from bitcoin_client.utils import bip32_path_from_string

from .wallet import AddressType, Wallet, MultisigWallet

MAX_APDU_LEN: int = 255


def chunkify(data: bytes, chunk_len: int) -> Iterator[Tuple[bool, bytes]]:
    size: int = len(data)

    if size <= chunk_len:
        yield True, data
        return

    chunk: int = size // chunk_len
    remaining: int = size % chunk_len
    offset: int = 0

    for i in range(chunk):
        yield False, data[offset:offset + chunk_len]
        offset += chunk_len

    if remaining:
        yield True, data[offset:]


class BitcoinInsType(enum.IntEnum):
    GET_PUBKEY = 0x00
    GET_ADDRESS = 0x01
    REGISTER_WALLET = 0x02
    GET_WALLET_ADDRESS = 0x03
    SIGN_PSBT = 0x04
    GET_SUM_OF_SQUARES = 0xF0

class FrameworkInsType(enum.IntEnum):
    CONTINUE_INTERRUPTED = 0x01


class ClientCommandCode(enum.IntEnum):
    GET_PUBKEY_INFO = 0x01
    GET_PUBKEYS_IN_DERIVATION_ORDER = 0x20
    GET_PREIMAGE = 0x40
    GET_MERKLE_LEAF_PROOF = 0x41
    GET_MERKLE_LEAF_INDEX = 0x42
    GET_MORE_ELEMENTS = 0xA0


class BitcoinCommandBuilder:
    """APDU command builder for the Bitcoin application.

    Parameters
    ----------
    debug: bool
        Whether you want to see logging or not.

    Attributes
    ----------
    debug: bool
        Whether you want to see logging or not.

    """
    CLA_BITCOIN: int = 0xE1
    CLA_FRAMEWORK: int = 0xFE

    def __init__(self, debug: bool = False):
        """Init constructor."""
        self.debug = debug

    def serialize(self,
                  cla: int,
                  ins: Union[int, enum.IntEnum],
                  p1: int = 0,
                  p2: int = 0,
                  cdata: bytes = b"") -> bytes:
        """Serialize the whole APDU command (header + data).

        Parameters
        ----------
        cla : int
            Instruction class: CLA (1 byte)
        ins : Union[int, IntEnum]
            Instruction code: INS (1 byte)
        p1 : int
            Instruction parameter 1: P1 (1 byte).
        p2 : int
            Instruction parameter 2: P2 (1 byte).
        cdata : bytes
            Bytes of command data.

        Returns
        -------
        bytes
            Bytes of a complete APDU command.

        """
        ins = cast(int, ins.value) if isinstance(ins, enum.IntEnum) else cast(int, ins)

        header: bytes = struct.pack("BBBBB",
                                    cla,
                                    ins,
                                    p1,
                                    p2,
                                    len(cdata))  # add Lc to APDU header

        if self.debug:
            logging.info("header: %s", header.hex())
            logging.info("cdata:  %s", cdata.hex())

        return header + cdata


    def get_pubkey(self, bip32_path: List[int], display: bool = False):
        bip32_paths: List[bytes] = bip32_path_from_string(bip32_path)

        cdata: bytes = b"".join([
            len(bip32_paths).to_bytes(1, byteorder="big"),
            *bip32_paths
        ])

        return self.serialize(cla=self.CLA_BITCOIN,
                        ins=BitcoinInsType.GET_PUBKEY,
                        p1=1 if display else 0,
                        cdata=cdata)

    def get_address(self, address_type: AddressType, bip32_path: List[int], display: bool = False):
        bip32_paths: List[bytes] = bip32_path_from_string(bip32_path)

        cdata: bytes = b"".join([
            len(bip32_paths).to_bytes(1, byteorder="big"),
            *bip32_paths
        ])

        return self.serialize(cla=self.CLA_BITCOIN,
                        p1=1 if display else 0,
                        p2=address_type,
                        ins=BitcoinInsType.GET_ADDRESS,
                        cdata=cdata)

    def register_wallet(self, wallet: Wallet):
        return self.serialize(cla=self.CLA_BITCOIN,
                        p1=0,
                        p2=0,
                        ins=BitcoinInsType.REGISTER_WALLET,
                        cdata=wallet.serialize())

    def get_wallet_address(self, wallet: Wallet, signature: bytes, address_index: int, display: bool = False):
        cdata: bytes = b"".join([
            wallet.id,
            len(signature).to_bytes(1, byteorder="big"),
            signature,
            wallet.serialize(),
            address_index.to_bytes(4, byteorder="big")
        ])
        return self.serialize(cla=self.CLA_BITCOIN,
                        p1=1 if display else 0,
                        ins=BitcoinInsType.GET_WALLET_ADDRESS,
                        cdata=cdata)


    # TODO: placeholder for the actual command, just for testing
    def sign_psbt(self, hash: bytes):
        if len(hash) != 20:
            raise ValueError("Lenght of hash should be 20 bytes.")

        return self.serialize(cla=self.CLA_BITCOIN,
                        ins=BitcoinInsType.SIGN_PSBT,
                        cdata=hash)


    def continue_interrupted(self, cdata: bytes):
        """Command builder for CONTINUE.

        Returns
        -------
        bytes
            APDU command for CONTINUE.

        """
        return self.serialize(cla=self.CLA_FRAMEWORK,
                        ins=FrameworkInsType.CONTINUE_INTERRUPTED,
                        cdata=cdata)
