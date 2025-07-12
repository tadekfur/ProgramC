# Production Deployment Guide - EtykietyManager

## 🚀 Production Deployment Checklist

### Pre-Deployment Security Audit

#### ✅ Security Components Verified
- [x] **Password Security**: PBKDF2 hashing with 10,000 iterations ✅
- [x] **Session Management**: Configurable timeout with auto-logout ✅
- [x] **Account Lockout**: 5 attempts, 15-minute lockout ✅
- [x] **Database Security**: Environment-based configuration ✅
- [x] **Connection Pooling**: 3 secure connections ✅
- [x] **Migration Tool**: Secure password migration ✅

#### ✅ Configuration Security
- [x] **No hardcoded credentials** in source code ✅
- [x] **Environment variables** for all secrets ✅
- [x] **Secure key generation** implemented ✅
- [x] **Session timeout** configurable ✅
- [x] **Database type** configurable ✅

## 🔧 Production Environment Setup

### 1. Server Requirements

#### Minimum Hardware
- **CPU**: 4 cores, 2.4 GHz
- **RAM**: 8 GB
- **Storage**: 50 GB SSD
- **Network**: 100 Mbps

#### Recommended Hardware
- **CPU**: 8 cores, 3.2 GHz
- **RAM**: 16 GB
- **Storage**: 100 GB NVMe SSD
- **Network**: 1 Gbps

### 2. Software Requirements

#### Operating System
- **Linux**: Ubuntu 20.04 LTS / CentOS 8 / RHEL 8
- **Windows**: Windows Server 2019/2022

#### Dependencies
```bash
# Ubuntu/Debian
sudo apt update && sudo apt install -y \
  qt6-base-dev \
  qt6-sql-dev \
  qt6-network-dev \
  qt6-pdf-dev \
  qt6-webengine-dev \
  postgresql-client \
  libpq-dev \
  cmake \
  build-essential

# CentOS/RHEL
sudo dnf install -y \
  qt6-qtbase-devel \
  qt6-qtbase-private-devel \
  postgresql-devel \
  cmake \
  gcc-c++
```

### 3. Database Setup

#### PostgreSQL Production Configuration
```bash
# Install PostgreSQL
sudo apt install postgresql postgresql-contrib

# Configure PostgreSQL
sudo -u postgres psql
CREATE DATABASE etykiety_db;
CREATE USER etykiety_user WITH ENCRYPTED PASSWORD 'STRONG_PASSWORD_HERE';
GRANT ALL PRIVILEGES ON DATABASE etykiety_db TO etykiety_user;
\\q

# Configure pg_hba.conf for secure connections
sudo vim /etc/postgresql/*/main/pg_hba.conf
# Add: host etykiety_db etykiety_user localhost md5

# Restart PostgreSQL
sudo systemctl restart postgresql
```

#### Database Security Hardening
```bash
# SSL/TLS Configuration
sudo vim /etc/postgresql/*/main/postgresql.conf
# Set: ssl = on
# Set: ssl_cert_file = 'server.crt'
# Set: ssl_key_file = 'server.key'

# Firewall Configuration
sudo ufw allow from 127.0.0.1 to any port 5432
sudo ufw deny 5432
```

### 4. Application Deployment

#### Create Production User
```bash
sudo useradd -r -s /bin/false etykiety
sudo mkdir -p /opt/etykiety
sudo chown etykiety:etykiety /opt/etykiety
```

#### Set Environment Variables
```bash
# Create secure environment file
sudo vim /etc/etykiety/environment
```

```bash
# /etc/etykiety/environment
DB_TYPE=QPSQL
DB_HOST=localhost
DB_PORT=5432
DB_NAME=etykiety_db
DB_USER=etykiety_user
DB_PASSWORD=STRONG_DATABASE_PASSWORD_HERE
APP_SECRET_KEY=STRONG_SECRET_KEY_256_BITS_HERE
SESSION_TIMEOUT=7200
PRODUCTION_MODE=true
LOG_LEVEL=WARNING
```

#### Secure Environment File
```bash
sudo chown root:etykiety /etc/etykiety/environment
sudo chmod 640 /etc/etykiety/environment
```

#### Deploy Application
```bash
# Build application
mkdir build && cd build
source /etc/etykiety/environment
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/etykiety
make -j$(nproc)
sudo make install

# Set permissions
sudo chown -R etykiety:etykiety /opt/etykiety
sudo chmod +x /opt/etykiety/bin/EtykietyManager
```

### 5. System Service Configuration

#### Create systemd service
```bash
sudo vim /etc/systemd/system/etykiety.service
```

```ini
[Unit]
Description=EtykietyManager Application
After=network.target postgresql.service
Requires=postgresql.service

[Service]
Type=simple
User=etykiety
Group=etykiety
WorkingDirectory=/opt/etykiety
ExecStart=/opt/etykiety/bin/EtykietyManager
EnvironmentFile=/etc/etykiety/environment
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

# Security hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/etykiety/data
CapabilityBoundingSet=CAP_NET_BIND_SERVICE
AmbientCapabilities=CAP_NET_BIND_SERVICE

[Install]
WantedBy=multi-user.target
```

#### Enable and start service
```bash
sudo systemctl daemon-reload
sudo systemctl enable etykiety.service
sudo systemctl start etykiety.service
sudo systemctl status etykiety.service
```

### 6. Security Hardening

#### Firewall Configuration
```bash
# UFW Configuration
sudo ufw --force enable
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow ssh
sudo ufw allow 8080/tcp  # Application port
sudo ufw allow from 127.0.0.1 to any port 5432
```

#### SSL/TLS Certificate
```bash
# Generate self-signed certificate (for testing)
openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365 -nodes

# Or use Let's Encrypt for production
sudo apt install certbot
sudo certbot certonly --standalone -d your-domain.com
```

#### Log Configuration
```bash
# Configure log rotation
sudo vim /etc/logrotate.d/etykiety
```

```
/var/log/etykiety/*.log {
    daily
    rotate 30
    compress
    delaycompress
    missingok
    notifempty
    create 644 etykiety etykiety
}
```

### 7. Monitoring and Backup

#### Application Monitoring
```bash
# Create monitoring script
sudo vim /usr/local/bin/etykiety-monitor.sh
```

```bash
#!/bin/bash
SERVICE_NAME="etykiety"
LOG_FILE="/var/log/etykiety/monitor.log"

if ! systemctl is-active --quiet $SERVICE_NAME; then
    echo "$(date): $SERVICE_NAME is not running, restarting..." >> $LOG_FILE
    systemctl restart $SERVICE_NAME
    
    # Send alert (implement your notification system)
    # mail -s "EtykietyManager Service Restarted" admin@company.com < $LOG_FILE
fi
```

#### Database Backup
```bash
# Create backup script
sudo vim /usr/local/bin/etykiety-backup.sh
```

```bash
#!/bin/bash
BACKUP_DIR="/opt/etykiety/backups"
DATE=$(date +%Y%m%d_%H%M%S)
DB_NAME="etykiety_db"

mkdir -p $BACKUP_DIR

# Database backup
pg_dump -h localhost -U etykiety_user $DB_NAME > $BACKUP_DIR/db_backup_$DATE.sql

# Application data backup
tar -czf $BACKUP_DIR/app_backup_$DATE.tar.gz /opt/etykiety/data

# Cleanup old backups (keep 30 days)
find $BACKUP_DIR -name "*.sql" -mtime +30 -delete
find $BACKUP_DIR -name "*.tar.gz" -mtime +30 -delete
```

#### Schedule Backups
```bash
# Add to crontab
sudo crontab -e
# Add: 0 2 * * * /usr/local/bin/etykiety-backup.sh
# Add: */5 * * * * /usr/local/bin/etykiety-monitor.sh
```

### 8. Performance Tuning

#### Database Optimization
```sql
-- PostgreSQL optimization
ALTER SYSTEM SET shared_buffers = '256MB';
ALTER SYSTEM SET effective_cache_size = '1GB';
ALTER SYSTEM SET maintenance_work_mem = '64MB';
ALTER SYSTEM SET checkpoint_completion_target = 0.7;
SELECT pg_reload_conf();
```

#### Application Configuration
```bash
# Optimize for production
export QT_QPA_PLATFORM=offscreen  # For headless deployment
export QT_LOGGING_RULES="*.debug=false"
export QT_QUICK_CONTROLS_MATERIAL_THEME=Dark
```

### 9. Testing Production Deployment

#### Pre-Production Testing
```bash
# Test database connection
psql -h localhost -U etykiety_user -d etykiety_db -c "SELECT version();"

# Test application startup
sudo -u etykiety /opt/etykiety/bin/EtykietyManager --test-mode

# Test user migration
sudo -u etykiety /opt/etykiety/bin/migrate_security --verify

# Test security features
sudo -u etykiety /opt/etykiety/bin/EtykietyManager --test-security
```

#### Load Testing
```bash
# Install testing tools
sudo apt install apache2-utils

# Test application endpoints
ab -n 1000 -c 10 http://localhost:8080/health

# Monitor resource usage
htop
iotop
```

### 10. Maintenance Procedures

#### Regular Maintenance Tasks
```bash
# Weekly maintenance script
sudo vim /usr/local/bin/etykiety-maintenance.sh
```

```bash
#!/bin/bash
echo "$(date): Starting maintenance..."

# Update system packages
apt update && apt upgrade -y

# Clean temporary files
find /tmp -name "etykiety_*" -mtime +7 -delete

# Analyze database
sudo -u postgres psql -d etykiety_db -c "ANALYZE;"

# Check disk space
df -h /opt/etykiety

# Check service status
systemctl status etykiety.service

echo "$(date): Maintenance completed"
```

#### Security Updates
```bash
# Monthly security update
sudo apt update && sudo apt upgrade -y
sudo systemctl restart etykiety.service

# Check for application updates
cd /opt/etykiety/source
git pull origin main
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo systemctl stop etykiety.service
sudo make install
sudo systemctl start etykiety.service
```

### 11. Troubleshooting Guide

#### Common Issues

**Service Won't Start**
```bash
# Check logs
sudo journalctl -u etykiety.service -f

# Check environment
sudo -u etykiety env

# Check permissions
ls -la /opt/etykiety/bin/EtykietyManager
```

**Database Connection Failed**
```bash
# Test connection
psql -h localhost -U etykiety_user -d etykiety_db

# Check PostgreSQL status
sudo systemctl status postgresql

# Check logs
sudo tail -f /var/log/postgresql/postgresql-*-main.log
```

**Performance Issues**
```bash
# Check resource usage
htop
iotop
netstat -tlnp

# Check database performance
sudo -u postgres psql -c "SELECT * FROM pg_stat_activity;"

# Check application logs
sudo tail -f /var/log/etykiety/application.log
```

### 12. Security Incident Response

#### Incident Response Plan
1. **Detection**: Monitor logs and alerts
2. **Containment**: Isolate affected systems
3. **Investigation**: Analyze security logs
4. **Recovery**: Restore from backups
5. **Lessons Learned**: Update security measures

#### Emergency Procedures
```bash
# Emergency shutdown
sudo systemctl stop etykiety.service
sudo systemctl stop postgresql

# Lock user accounts
sudo -u etykiety /opt/etykiety/bin/EtykietyManager --lock-all-accounts

# Create forensic backup
sudo dd if=/dev/sda of=/forensic/system_backup.img bs=4M

# Restore from backup
sudo systemctl stop etykiety.service
sudo -u postgres psql -c "DROP DATABASE etykiety_db;"
sudo -u postgres psql -c "CREATE DATABASE etykiety_db;"
sudo -u postgres psql etykiety_db < /opt/etykiety/backups/db_backup_latest.sql
sudo systemctl start etykiety.service
```

## 📋 Final Deployment Checklist

### Pre-Deployment
- [ ] Environment variables configured
- [ ] Database setup and secured
- [ ] SSL certificates installed
- [ ] Firewall configured
- [ ] Service user created
- [ ] Application built and tested

### Deployment
- [ ] Application deployed to production path
- [ ] Systemd service created and enabled
- [ ] Monitoring scripts installed
- [ ] Backup procedures configured
- [ ] Log rotation configured

### Post-Deployment
- [ ] Service running and accessible
- [ ] Database connections working
- [ ] User authentication working
- [ ] Session management working
- [ ] Monitoring alerts configured
- [ ] Backup verification completed

### Security Verification
- [ ] No hardcoded credentials in code
- [ ] Environment variables secured
- [ ] Database access restricted
- [ ] Network access controlled
- [ ] Logs properly configured
- [ ] Security updates scheduled

## 🎉 Deployment Complete!

Your EtykietyManager application is now securely deployed in production with:
- ✅ **Enterprise-grade security**
- ✅ **Automated monitoring**
- ✅ **Regular backups**
- ✅ **Performance optimization**
- ✅ **Incident response procedures**

For support or issues, refer to the troubleshooting guide or contact your system administrator.