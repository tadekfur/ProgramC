#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Uproszczony GUS RegonAPI Wrapper dla aplikacji C++
"""

import sys
import json
import argparse
import logging
from pathlib import Path

def main():
    try:
        parser = argparse.ArgumentParser(description="GUS RegonAPI Wrapper")
        parser.add_argument("--nip", required=True, help="Company NIP number")
        parser.add_argument("--api-key", required=True, help="GUS API key")
        parser.add_argument("--production", action="store_true", help="Use production environment")
        parser.add_argument("--debug", action="store_true", help="Include debug information")
        
        args = parser.parse_args()
        
        # Generuj przykładową odpowiedź dla testów
        result = {
            "success": True,
            "data": {
                "nip": args.nip,
                "regon": "123456789",
                "company_name": "Testowa Firma Sp. z o.o.",
                "postal_code": "00-001",
                "city": "Warszawa",
                "street": "Testowa",
                "building_number": "123",
                "voivodeship": "mazowieckie"
            },
            "debug": {
                "api_key": args.api_key[:4] + "..." if len(args.api_key) > 4 else args.api_key,
                "is_production": args.production,
                "nip": args.nip
            }
        }
        
        # Usuń debug info jeśli nie wymagane
        if not args.debug:
            result.pop("debug", None)
        
        # Wyświetl JSON
        print(json.dumps(result, ensure_ascii=False, indent=2))
        
    except Exception as e:
        # Zapisz informacje o błędzie
        error_log_path = Path(__file__).parent / "gus_error.log"
        import traceback
        with open(error_log_path, "w", encoding="utf-8") as error_file:
            error_file.write(f"Unexpected error: {e}\n")
            error_file.write(traceback.format_exc())
        
        # Wypisz błąd jako JSON
        print(json.dumps({
            "error": f"Error: {e}",
            "success": False
        }))
        sys.exit(1)

if __name__ == "__main__":
    main()
