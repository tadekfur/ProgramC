#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GUS RegonAPI Wrapper for C++ Application
Provides command-line interface for fetching company data from GUS.
"""

import sys
import json
import argparse
import logging
from pathlib import Path

# Add RegonAPI to path - sprawdź kilka możliwych ścieżek
possible_paths = [
    Path(__file__).parent.parent / "RegonAPI-1.3.1",  # Standardowa ścieżka
    Path(__file__).parent.parent.parent / "RegonAPI-1.3.1",  # Dla przypadku build/python/gus_wrapper.py
    Path(__file__).parent.parent.parent.parent / "RegonAPI-1.3.1",  # Dla głębszych zagnieżdżeń
    Path("C:/ProgramC/RegonAPI-1.3.1")  # Bezpośrednia ścieżka
]

# Znajdź pierwszą istniejącą ścieżkę
regon_api_path = None
for path in possible_paths:
    if path.exists():
        regon_api_path = path
        break

if not regon_api_path:
    error_msg = f"Nie można znaleźć RegonAPI-1.3.1. Sprawdzone ścieżki: {[str(p) for p in possible_paths]}"
    print(json.dumps({
        "error": error_msg,
        "success": False
    }))
    sys.exit(1)

# Dodaj znalezioną ścieżkę do sys.path
sys.path.insert(0, str(regon_api_path))

try:
    from RegonAPI import RegonAPI
except ImportError as e:
    # Zapisz szczegóły błędu do pliku dla lepszej diagnostyki
    error_log_path = Path(__file__).parent / "gus_error.log"
    import traceback
    with open(error_log_path, "w", encoding="utf-8") as error_file:
        error_file.write(f"Failed to import RegonAPI: {e}\n")
        error_file.write(f"Python path: {sys.path}\n")
        error_file.write(f"Looking for RegonAPI at: {regon_api_path}\n")
        error_file.write(f"RegonAPI path exists: {regon_api_path.exists()}\n")
        error_file.write(f"Current working directory: {Path.cwd()}\n")
        error_file.write(f"__file__: {__file__}\n")
        error_file.write(traceback.format_exc())
    
    # Wypisz błąd na stdout w formacie JSON
    print(json.dumps({
        "error": f"Failed to import RegonAPI: {e}. See gus_error.log for details.",
        "success": False
    }))
    sys.exit(1)

# Configure logging to suppress verbose output
logging.getLogger().setLevel(logging.ERROR)

def fetch_company_data(nip, api_key, is_production=True):
    """
    Fetch company data from GUS using RegonAPI
    
    Args:
        nip (str): Company NIP number
        api_key (str): GUS API key
        is_production (bool): Use production environment if True
    
    Returns:
        dict: Response with company data or error information
    """
    result = {
        "success": False,
        "data": {},
        "error": None,
        "debug": {}
    }
    
    try:
        # Clean NIP - remove all non-digit characters
        clean_nip = ''.join(filter(str.isdigit, nip))
        
        if len(clean_nip) != 10:
            result["error"] = f"Invalid NIP length: {len(clean_nip)} (expected 10 digits)"
            return result
        
        result["debug"]["clean_nip"] = clean_nip
        result["debug"]["api_key"] = api_key[:8] + "..." if len(api_key) > 8 else api_key
        result["debug"]["is_production"] = is_production
        
        # Initialize RegonAPI client
        regon_api = RegonAPI(
            bir_version="bir1.1",
            is_production=is_production,
            timeout=30,
            operation_timeout=30
        )
        
        result["debug"]["service_url"] = regon_api.service_url
        result["debug"]["wsdl"] = regon_api.wsdl
        
        # Authenticate
        sid = regon_api.authenticate(api_key)
        result["debug"]["sid"] = sid[:8] + "..." if sid and len(sid) > 8 else sid
        
        # Search for company data
        search_results = regon_api.searchData(nip=clean_nip)
        
        if not search_results:
            result["error"] = "No company found for the provided NIP"
            return result
        
        # Get the first result (should be only one for NIP search)
        company_basic = search_results[0]
        result["debug"]["basic_data_keys"] = list(company_basic.keys())
        
        # Get detailed report
        regon = company_basic.get("Regon")
        if not regon:
            result["error"] = "No REGON found in search results"
            result["data"] = company_basic
            return result
        
        # Determine report type based on company type
        company_type = company_basic.get("Typ", "")
        if company_type == "F":  # Fizyczna
            report_name = "BIR11OsFizycznaDaneOgolne"
        elif company_type == "P":  # Prawna
            report_name = "BIR11OsPrawna"
        else:
            # Default to legal entity report
            report_name = "BIR11OsPrawna"
        
        result["debug"]["report_name"] = report_name
        
        try:
            detailed_data = regon_api.dataDownloadFullReport(regon, report_name)
            if detailed_data:
                # Merge basic and detailed data
                company_data = {**company_basic, **detailed_data[0]}
            else:
                company_data = company_basic
        except Exception as report_error:
            result["debug"]["report_error"] = str(report_error)
            company_data = company_basic
        
        # Clean and standardize field names
        standardized_data = {}
        field_mapping = {
            "Nazwa": "company_name",
            "NazwaNpodmiotu": "company_name",
            "NazwaOrg": "company_name", 
            "Nip": "nip",
            "Regon": "regon",
            "Krs": "krs",
            "Wojewodztwo": "voivodeship",
            "Powiat": "county",
            "Gmina": "commune",
            "Miejscowosc": "city",
            "KodPocztowy": "postal_code",
            "Ulica": "street",
            "NrNieruchomosci": "building_number",
            "NrLokalu": "apartment_number",
            "Typ": "entity_type",
            "SilosID": "silos_id",
            "DataZakonczeniaDzialalnosci": "business_end_date",
            "MiejscowoscPoczty": "postal_city"
        }
        
        for original_key, value in company_data.items():
            if value and str(value).strip():  # Only include non-empty values
                mapped_key = field_mapping.get(original_key, original_key.lower())
                standardized_data[mapped_key] = str(value).strip()
        
        result["success"] = True
        result["data"] = standardized_data
        result["debug"]["total_fields"] = len(standardized_data)
        
        return result
        
    except Exception as e:
        result["error"] = f"RegonAPI error: {str(e)}"
        result["debug"]["exception_type"] = type(e).__name__
        return result

def main():
    try:
        parser = argparse.ArgumentParser(description="GUS RegonAPI Wrapper")
        parser.add_argument("--nip", required=True, help="Company NIP number")
        parser.add_argument("--api-key", required=True, help="GUS API key")
        parser.add_argument("--production", action="store_true", help="Use production environment")
        parser.add_argument("--debug", action="store_true", help="Include debug information")
        
        args = parser.parse_args()
        
        result = fetch_company_data(args.nip, args.api_key, args.production)
        
        # Remove debug info if not requested
        if not args.debug:
            result.pop("debug", None)
        
        # Output JSON result
        print(json.dumps(result, ensure_ascii=False, indent=2))
    except Exception as e:
        # Zapisz szczegóły błędu do pliku dla lepszej diagnostyki
        error_log_path = Path(__file__).parent / "gus_error.log"
        import traceback
        with open(error_log_path, "w", encoding="utf-8") as error_file:
            error_file.write(f"Unexpected error: {e}\n")
            error_file.write(f"Python path: {sys.path}\n")
            error_file.write(f"Current working directory: {Path.cwd()}\n")
            error_file.write(f"__file__: {__file__}\n")
            error_file.write(traceback.format_exc())
        
        # Wypisz błąd na stdout w formacie JSON
        print(json.dumps({
            "error": f"Unexpected error: {e}. See gus_error.log for details.",
            "success": False
        }), file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
