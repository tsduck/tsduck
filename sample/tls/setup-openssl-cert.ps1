#
# Setup a test key and certificate.
# The appropriate environment variables are defined.
#

# Name of the key.
$KeyName = "test_key"

# Look for existing key.
$cert = Get-ChildItem Cert:\CurrentUser\My | Where-Object -Property FriendlyName -eq $KeyName

if ($cert -eq $null) {
    Write-Output "Key $KeyName not found, creating one"
    $cert = New-SelfSignedCertificate -FriendlyName $KeyName -Type SSLServerAuthentication `
                -DnsName @([System.Net.Dns]::GetHostName(), "localhost") `
                -CertStoreLocation Cert:\CurrentUser\My `
                -KeyAlgorithm "RSA" -KeyLength 3072 `
                -NotAfter ((Get-Date).AddYears(5))
}

# Display the certificate.
$cert | Format-List

# Define environment variables for default values.
$env:TSDUCK_TLS_CERTIFICATE = $KeyName
