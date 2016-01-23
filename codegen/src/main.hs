import System.Environment
import Data.List

import Text.ParserCombinators.Parsec

data GenDefinition = ComponentDefinition String [GenEntryDefinition]
                   | SystemDefinition String [GenEntryDefinition]
                   deriving(Show, Eq)

data GenEntryDefinition = MemberList [VariableDeclaration]
                        | FunctionList [FunctionEntry]
                        | Family [FamilyComponentEntry]
                        | Option String String
                        deriving(Show, Eq)
                        
type VariableDeclaration = (String, String)
type FunctionSignature = ([VariableDeclaration], String)
type FunctionEntry = (String, FunctionSignature)
type FamilyComponentEntry = String

nonBreakingChars = " \t"
newLineChars = "\n\r"
whiteSpaceChars = nonBreakingChars ++ newLineChars
reservedChars = "{}:=(),"

identifier = many1 (noneOf (whiteSpaceChars ++ reservedChars))
typeName = many1 (noneOf (newLineChars ++ reservedChars))
restOfLine = many1 (noneOf (newLineChars ++ "{}"))

whiteSpace = many (oneOf whiteSpaceChars)
nonBreakingWhiteSpace = many (oneOf nonBreakingChars)

eol =   try (string "\n\r")
    <|> try (string "\r\n")
    <|> string "\n"
    <|> string "\r"

outerEntry typeName c f = do
    whiteSpace
    string typeName
    whiteSpace
    genName <- identifier
    whiteSpace
    char '='
    whiteSpace
    contents <- block c
    nonBreakingWhiteSpace
    many eol
    return $ f genName contents

innerEntry typeName p = do
    whiteSpace
    string typeName
    whiteSpace
    char '='
    whiteSpace
    result <- p
    nonBreakingWhiteSpace
    many eol
    return result

block :: GenParser Char st a -> GenParser Char st [a]
block c = do
    char '{'
    contents <- many p
    char '}'
    whiteSpace
    return contents
    where
        p = do
            whiteSpace
            result <- c
            whiteSpace
            return result

variable = do
    whiteSpace
    name <- identifier <?> "identifier (name)"
    whiteSpace
    char ':'
    whiteSpace
    varType <- typeName <?> "identifier (type)"
    nonBreakingWhiteSpace
    return (name, varType)
    
member = do
    result <- variable
    eol <?> "end of line after member"
    return result

function = do
    whiteSpace
    name <- identifier <?> "identifier (function name)"
    whiteSpace
    char ':'
    whiteSpace
    signature <- functionSignature
    nonBreakingWhiteSpace
    eol <?> "end of line after function"
    return (name, signature)
    
functionSignature = do
    whiteSpace
    char '('
    args <- sepBy variable (char ',')
    char ')'
    whiteSpace
    string "->"
    whiteSpace
    returnType <- typeName <?> "function return type"
    return (args, returnType)
    
componentInFamily = do
    whiteSpace
    name <- restOfLine
    nonBreakingWhiteSpace
    eol <?> "end of line after component"
    return name
    
memberList = innerEntry "members" (do { block <- block member ; return $ MemberList block })
functionList = innerEntry "functions" (do { block <- block function ; return $ FunctionList block })
familyList = innerEntry "family" (do { block <- block componentInFamily ; return $ Family block })
specificOption n = innerEntry n (do { opt <- identifier; return $ Option n opt })

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

componentDef = outerEntry "component" (componentEntries) (\n ges -> ComponentDefinition n ges)
systemDef = outerEntry "system" (systemEntries) (\n ges -> SystemDefinition n ges)

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