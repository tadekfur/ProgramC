-- Migracja: nowe tabele dla zamówień materiałów od dostawców
-- suppliers: słownik dostawców
CREATE TABLE IF NOT EXISTS suppliers (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    street VARCHAR(255),
    city VARCHAR(100),
    postal_code VARCHAR(20),
    country VARCHAR(100),
    contact_person VARCHAR(100),
    phone VARCHAR(50),
    email VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- materials_catalog: słownik materiałów
CREATE TABLE IF NOT EXISTS materials_catalog (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    width VARCHAR(50),
    length VARCHAR(50),
    unit VARCHAR(20),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- materials_orders: zamówienia materiałów
CREATE TABLE IF NOT EXISTS materials_orders (
    id SERIAL PRIMARY KEY,
    order_number VARCHAR(32) NOT NULL UNIQUE,
    order_date DATE NOT NULL,
    delivery_date DATE,
    notes TEXT,
    supplier_id INTEGER REFERENCES suppliers(id),
    delivery_company VARCHAR(255),
    delivery_street VARCHAR(255),
    delivery_city VARCHAR(100),
    delivery_postal_code VARCHAR(20),
    delivery_country VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- materials_order_items: pozycje zamówienia materiałów
CREATE TABLE IF NOT EXISTS materials_order_items (
    id SERIAL PRIMARY KEY,
    order_id INTEGER REFERENCES materials_orders(id) ON DELETE CASCADE,
    material_id INTEGER REFERENCES materials_catalog(id),
    material_name VARCHAR(255), -- dla materiałów spoza słownika
    width VARCHAR(50),
    length VARCHAR(50),
    quantity VARCHAR(50),
    unit VARCHAR(20),
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Sekwencja do numeracji zamówień materiałów (np. TER/YY/MM/XXXX)
CREATE SEQUENCE IF NOT EXISTS materials_order_seq START 1;
