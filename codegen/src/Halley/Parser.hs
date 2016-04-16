{-
   Copyright 2016 Rodrigo Braz Monteiro

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
-}
module Halley.Parser
( parseFile
) where

import Halley.AST
import Text.ParserCombinators.Parsec
import Data.List


--------------------------


parseFile :: String -> String -> Either ParseError [GenDefinition]
parseFile filename input = parse genDefs filename input


-------------------------

nonBreakingChars = " \t"
newLineChars = "\n\r"
whiteSpaceChars = nonBreakingChars ++ newLineChars
reservedChars = "{}=(),"

whiteSpace = many (oneOf whiteSpaceChars)
nonBreakingWhiteSpace = many (oneOf nonBreakingChars)

eol =   try (string "\n\r")
    <|> try (string "\r\n")
    <|> string "\n"
    <|> string "\r"
    <?> "end of line"

-- Matches whitespace, but must contain at least one EOF    
whiteSpaceWithEol = do
    nonBreakingWhiteSpace
    eol
    whiteSpace

-- Trims whitespace around p
trim p = try (do
    nonBreakingWhiteSpace
    p <* nonBreakingWhiteSpace)



-------------------------


identifier = many1 (noneOf (whiteSpaceChars ++ reservedChars)) <?> "identifier"
typeName = many1 (noneOf (newLineChars ++ reservedChars)) <?> "type name"

namedBlock typeName c f = do
    string typeName
    genName <- trim (identifier)
    contents <- block c
    return $ f genName contents

anonymousBlock typeName c f = do
    string typeName
    whiteSpace
    contents <- block c
    return $ f contents

equalsEntry typeName p = do
    string typeName
    trim (char '=')
    p

block c = char '{' >> (entrySequence c) <* (char '}')
entrySequence c = whiteSpace >> many (c <* whiteSpaceWithEol)

variable = do
    name <- identifier
    trim (char ':')
    varType <- typeName
    return (name, varType)
    
function = do
    name <- identifier
    trim (char ':')
    signature <- functionSignature
    return (name, signature)

functionSignature = do
    char '('
    args <- sepBy (trim variable) (char ',')
    char ')'
    trim (string "->")
    returnType <- typeName
    return (args, returnType)

componentDef = namedBlock "component" (componentEntries) (\n ges -> ComponentDefinition n ges)
systemDef = namedBlock "system" (systemEntries) (\n ges -> SystemDefinition n ges)
memberList = anonymousBlock "members" (variable) (\ges -> MemberList ges)
functionList = anonymousBlock "functions" (function) (\ges -> FunctionList ges)
specificOption n = equalsEntry n (do { opt <- identifier; return $ Option n opt })
familyList = namedBlock "family" (typeName) (\n ges -> Family n ges)

{-
familyList = do
    string "family"
    whiteSpace
    name <- trim (identifier)
    block <- block typeName
    return $ Family name block
-}

optionEntry = do
    try (specificOption "strategy")
    <|> try (specificOption "method")
    <|> try (specificOption "access")
    <|> try (specificOption "language")
    <|> try (specificOption "timeline")
    <|> specificOption "subDivide"

componentEntries = do
    memberList
    <|> functionList

systemEntries = do
    familyList
    <|> optionEntry


genDef = do
    try componentDef
    <|> systemDef

genDefs = (entrySequence genDef) <* eof
