$file = "C:\Users\Felip\Downloads\Servidor 2\pawno\include\phone_core.inc"
$raw = [System.IO.File]::ReadAllBytes($file)
$content = [System.Text.Encoding]::UTF8.GetString($raw)

# Remover BOM se existir
if ($content.Length -gt 0 -and [int][char]$content[0] -eq 65279) {
    $content = $content.Substring(1)
}

# Remover includes relativos
$content = [System.Text.RegularExpressions.Regex]::Replace($content, '#include\s+"[^"]*"[^\r\n]*', '// include interno removido')

# Fix encoding corrompido - ordem importa, mais longo primeiro
$content = $content.Replace('Ã£o', 'ao')
$content = $content.Replace('Ã§Ã£', 'ca')
$content = $content.Replace('Ã§a', 'ca')
$content = $content.Replace('Ã§o', 'co')
$content = $content.Replace('Ã§u', 'cu')
$content = $content.Replace('Ã£', 'a')
$content = $content.Replace('Ã¡', 'a')
$content = $content.Replace('Ã ', 'a')
$content = $content.Replace('Ã¢', 'a')
$content = $content.Replace('Ã©', 'e')
$content = $content.Replace('Ãª', 'e')
$content = $content.Replace('Ã«', 'e')
$content = $content.Replace('Ã¨', 'e')
$content = $content.Replace('Ã­', 'i')
$content = $content.Replace('Ã®', 'i')
$content = $content.Replace('Ã¯', 'i')
$content = $content.Replace('Ã³', 'o')
$content = $content.Replace('Ã´', 'o')
$content = $content.Replace('Ã¶', 'o')
$content = $content.Replace('Ãµ', 'o')
$content = $content.Replace('Ãº', 'u')
$content = $content.Replace('Ã»', 'u')
$content = $content.Replace('Ã¼', 'u')
$content = $content.Replace('Ã§', 'c')
$content = $content.Replace('Ã±', 'n')
$content = $content.Replace('Ã‰', 'E')
$content = $content.Replace('Ã"', 'O')
$content = $content.Replace('Ãš', 'U')
$content = $content.Replace('Ã‚', 'A')
$content = $content.Replace('Ã‡', 'C')
$content = $content.Replace('â€"', '-')
$content = $content.Replace('â€™', "'")
$content = $content.Replace('â€œ', '"')
$content = $content.Replace('â€', '"')
$content = $content.Replace('Ãµes', 'oes')
$content = $content.Replace('Ã©ncia', 'encia')

$enc = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText($file, $content, $enc)
Write-Host "OK - tamanho: $((Get-Item $file).Length)"
