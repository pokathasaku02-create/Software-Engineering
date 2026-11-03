# Installation Guide — Moderated LMS v1.0

**Audience:** School IT technician.
**Reference:** Deployment diagram docs/diagrams/fig_7a_deployment.png (Moderated LMS v1.0).

This guide assumes a single Ubuntu Linux 22.04 (or later) school computer, as shown in the deployment diagram. The system works fully offline; no internet connection is needed after installation.

## 1. Hardware and software prerequisites

| Item | Requirement |
|---|---|
| Computer | 1 PC, 2 GB RAM, 100 MB free disk (diagram: "School computer") |
| Operating system | Ubuntu 22.04 LTS or later |
| Display | Keyboard and screen (console window) |
| Optional | Staff laptop with SSH access on the school network (diagram: "Staff laptop") |
| Optional | USB drive or second disk for backups (diagram: "Backup medium") |

You will need administrator (sudo) rights on the school computer.

## 2. Install the required libraries

Run:
sudo apt update
sudo apt install -y build-essential cmake pkg-config libsqlite3-dev libsodium-dev

These provide libsqlite3.so (database) and libsodium.so (password hashing), as shown in the deployment diagram.

## 3. Build the application

The full build procedure is in the top-level README.md (section 1). In short, run:
git clone <your-repository-url> lms && cd lms
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

The executable is build/lms.

## 4. Install to the locations shown in the deployment diagram

The deployment diagram places the application at /opt/lms/bin/lms and the database at /var/lib/lms/lms.db. Create those directories and copy the binary by running:

sudo mkdir -p /opt/lms/bin
sudo cp build/lms /opt/lms/bin/lms
sudo chmod 755 /opt/lms/bin/lms
sudo mkdir -p /var/lib/lms

To create the launcher script at /usr/local/bin/lms (as shown in the diagram), create a file with these contents:
#!/bin/sh
exec /opt/lms/bin/lms /var/lib/lms/lms.db "$@"

Then make it executable:
sudo chmod 755 /usr/local/bin/lms

## 5. Set database permissions (security)

The database holds learners' personal information. The deployment diagram specifies folder mode 2770, group lms, and backup files mode 700. Run:

sudo groupadd -f lms
sudo usermod -aG lms <technician-username>
sudo chown root:lms /var/lib/lms
sudo chmod 2770 /var/lib/lms

Confirm the result with: ls -ld /var/lib/lms
Expected: drwxrws--- ... root lms ... /var/lib/lms

## 6. First run — create the administrator account

There is no default password. On first run, if the database has no users, the program asks you to create the administrator account. Run: lms

1. Enter an administrator username and a strong password when prompted.
2. Log in as that administrator.
3. Use the Administrator menu to create departments, courses, lecturers and Heads of Department, students, and enrol students on courses.

The database file /var/lib/lms/lms.db is created automatically on first run.

Tip: After creating the administrator, run the program as a member of the lms group so that the database keeps the correct group ownership. If you created it with sudo, fix ownership with:
sudo chown root:lms /var/lib/lms/lms.db && sudo chmod 660 /var/lib/lms/lms.db

## 7. Set up backups

The deployment diagram shows a backup script (/usr/local/bin/lms-backup) writing dated copies (lms-DATE-TIME.db) to a USB drive or second disk, copied by cron or by hand.

Create the script file with this content (change BACKUP_DIR to your mounted USB path):
#!/bin/sh
BACKUP_DIR=/media/backup
STAMP=$(date +%Y-%m-%d-%H%M%S)
mkdir -p "$BACKUP_DIR"
cp /var/lib/lms/lms.db "$BACKUP_DIR/lms-$STAMP.db"
chmod 700 "$BACKUP_DIR/lms-$STAMP.db"

Then make it executable:
sudo chmod 755 /usr/local/bin/lms-backup

Schedule a daily backup with cron by running "sudo crontab -e" and adding:
0 18 * * * /usr/local/bin/lms-backup

Backup files are mode 700 (owner only), as required by the deployment diagram.

## 8. Optional: access from a staff laptop over SSH

Several staff members can use the system in turn, or at the same time through SSH. Install and enable the SSH server:
sudo apt install -y openssh-server
sudo systemctl enable --now ssh

Staff connect with ssh <username>@<school-computer-address> and then run lms. SQLite allows only one writer at a time; a second writer waits up to 5 seconds.

## 9. Verify the installation

| Check | Expected result |
|---|---|
| lms starts | Login prompt appears |
| Log in as administrator | Administrator menu appears |
| Create a department and course | Entries appear in the menus |
| ls -ld /var/lib/lms | drwxrws--- root lms |
| ls -l /var/lib/lms/lms.db | Owner/group readable/writable, others no access |
| Run lms-backup | A dated .db file appears on the backup medium, mode 700 |

## 10. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| libsqlite3.so or libsodium.so not found | Install libsqlite3-dev and libsodium-dev (section 2). |
| "database is locked" | Two users writing at once; the second writer waits up to 5 seconds. Retry. |
| Program cannot write the database | Check group ownership and mode 2770 on /var/lib/lms (section 5). |
| Password prompt is visible | This is normal on some consoles; it is hidden on Linux/macOS terminals. |
| Backup file missing | Check the BACKUP_DIR path in /usr/local/bin/lms-backup and the cron entry. |

For build problems (compiler, CMake, vcpkg) see README.md section 1.
