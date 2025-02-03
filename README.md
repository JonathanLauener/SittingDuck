# SittingDuck
A tool that waits for a Domain Admin to log in and then executes commands on behalf of said Admin

# MonitorLogins
How it Works:
* The program creates a hidden window to receive session change messages.
* WTSRegisterSessionNotification() makes the window receive WM_WTSSESSION_CHANGE messages.
* When a new user logs in, it receives WTS_SESSION_LOGON, it calls CheckPermissions, then (if the permissions are valid) it calls ImpersonateUser.

# CheckPermissions
How it Works:
* The function calls NetUserGetLocalGroups to obtain the local group memberships of the specified user (username) from the local machine, including both direct and indirect memberships.
*  The function checks if the user is a member of the group named "DAdmin", using wcsstr to perform a case-sensitive substring search on the group names returned. I use the NetUserGetLocalGroups because it is simpler to debug because my machine is not joined to a domain. I will change this once I get the chance to test this in an AD environment. For this to work you will need to make a new user and add them to the "DAdmin" group.

# ImpersonateUser
How it Works:
* The function first retrieves the process ID of the target user (targetUsername) by calling `GetExplorerPidByUser`, then attempts to open the target process to gain access to its authentication token using `OpenProcessToken`
