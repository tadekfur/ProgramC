-- Dodaj tabelę do przechowywania wszystkich użytych numerów klientów
CREATE TABLE IF NOT EXISTS used_client_numbers (
    client_number INTEGER PRIMARY KEY
);

-- Dodaj do tej tabeli wszystkie już istniejące numery klientów (jednorazowo)
INSERT INTO used_client_numbers (client_number)
SELECT DISTINCT client_number::integer FROM clients
ON CONFLICT DO NOTHING;
