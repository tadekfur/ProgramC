-- Dodanie kolumny status do tabeli orders
ALTER TABLE orders ADD COLUMN status INTEGER NOT NULL DEFAULT 0;
-- 0 = przyjęte do realizacji, 1 = produkcja, 2 = gotowe do wysyłki
