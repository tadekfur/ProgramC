-- Synchronizuj tabelę used_client_numbers z faktycznymi klientami
INSERT INTO used_client_numbers (client_number)
SELECT client_number::integer FROM clients
ON CONFLICT DO NOTHING;

-- Usuń z used_client_numbers numery, które nie istnieją już w clients (opcjonalnie, jeśli chcesz czyścić)
-- DELETE FROM used_client_numbers WHERE client_number NOT IN (SELECT client_number::integer FROM clients);
