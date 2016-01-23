import System.Environment
import Data.List

import Text.ParserCombinators.Parsec

data GenDefinition = ComponentDefinition String [GenEntryDefinition]
                   | SystemDefinition String [GenEntryDefinition]
                   deriving(Show, Eq)

data GenEntryDefinition = MemberList [MemberEntry]
                        | FunctionList [FunctionEntry]
                        | Family [FamilyComponentEntry]
                        | Option String String
                        deriving(Show, Eq)
                        
type MemberEntry = (String, String)
type FunctionEntry = (String, String)
type FamilyComponentEntry = String

whiteSpaceChars = " \n\r\t"

identifier = many1 (noneOf (whiteSpaceChars ++ "{}:="))
restOfLine = many1 (noneOf ("\r\n{}"))
            
whiteSpace = many (oneOf whiteSpaceChars)
nonBreakingWhiteSpace = many (oneOf " \t")

eol =   try (string "\n\r")
    <|> try (string "\r\n")
    <|> string "\n"
    <|> string "\r"

innerBlock :: GenParser Char st a -> GenParser Char st [a]
innerBlock c = do
    char '{'
    contents <- many p
    char '}'
    return contents
    where
        p = do
            whiteSpace
            result <- c
            whiteSpace
            return result

genDefGeneric genTypeName c f = do
    whiteSpace
    string genTypeName
    whiteSpace
    genName <- identifier
    whiteSpace
    char '{'
    contents <- many p
    char '}'
    whiteSpace
    return $ f genName contents
    where
        p = do
            whiteSpace
            result <- c
            whiteSpace
            return result

entry name p = do
    string name
    whiteSpace
    char '='
    whiteSpace
    result <- p
    nonBreakingWhiteSpace
    eol <?> "end of line after block"
    return result

member = do
    name <- identifier <?> "identifier (member name)"
    whiteSpace
    char ':'
    whiteSpace
    memberType <- identifier <?> "identifier (member type)"
    nonBreakingWhiteSpace
    eol <?> "end of line after member"
    return (name, memberType)
    
function = do
    name <- identifier <?> "identifier (function name)"
    whiteSpace
    char ':'
    whiteSpace
    memberType <- restOfLine
    nonBreakingWhiteSpace
    eol <?> "end of line after function"
    return (name, memberType)
    
componentInFamily = do
    name <- restOfLine
    nonBreakingWhiteSpace
    eol <?> "end of line after component"
    return name
    
memberList = do
    entry "members" p
    where
        p = do
            block <- innerBlock member
            return $ MemberList block

functionList = do
    entry "functions" p
    where
        p = do
            block <- innerBlock function
            return $ FunctionList block
            
familyList = do
    entry "family" p
    where
        p = do
            block <- innerBlock componentInFamily
            return $ Family block

specificOption n = do
    entry n p
    where
        p = do
            opt <- identifier
            return $ Option n opt

optionEntry = do
    try (specificOption "strategy")
    <|> try (specificOption "access")
    <|> try (specificOption "language")
    <|> try (specificOption "time")
    <|> specificOption "subDivide"

componentEntries = do
    memberList
    <|> functionList
    
systemEntries = do
    try familyList
    <|> optionEntry
        
componentDef = genDefGeneric "component" (componentEntries) (\n ges -> ComponentDefinition n ges)
systemDef = genDefGeneric "system" (systemEntries) (\n ges -> SystemDefinition n ges)
    
genDef = do
    try componentDef
    <|> systemDef

genDefs = do
    result <- many genDef
    eof
    return result

parseFile :: String -> Either ParseError [GenDefinition]
parseFile input = parse genDefs "(unknown)" input

main = do
    args <- getArgs
    rawFile <- readFile (head args)
    putStrLn $ case parseFile rawFile of
        Left error -> "Error: " ++ (show error)
        Right defs -> show defs