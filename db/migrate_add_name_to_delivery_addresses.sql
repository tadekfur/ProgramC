-- Dodaj kolumnę 'name' do tabeli delivery_addresses, jeśli nie istnieje
ALTER TABLE delivery_addresses ADD COLUMN IF NOT EXISTS name VARCHAR(255);
