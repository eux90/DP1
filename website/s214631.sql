-- phpMyAdmin SQL Dump
-- version 4.6.5.2
-- https://www.phpmyadmin.net/
--
-- Host: 127.0.0.1
-- Creato il: Giu 13, 2017 alle 18:18
-- Versione del server: 10.1.21-MariaDB
-- Versione PHP: 5.6.30

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `s214631`
--

-- --------------------------------------------------------

--
-- Struttura della tabella `bid`
--

DROP TABLE IF EXISTS `bid`;
CREATE TABLE `bid` (
  `username` varchar(254) NOT NULL,
  `amount` double NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dump dei dati per la tabella `bid`
--

INSERT INTO `bid` (`username`, `amount`) VALUES
('nobody', 1);

-- --------------------------------------------------------

--
-- Struttura della tabella `users`
--

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
  `username` varchar(254) NOT NULL,
  `password` char(64) NOT NULL,
  `thr` double DEFAULT NULL,
  `date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dump dei dati per la tabella `users`
--

INSERT INTO `users` (`username`, `password`, `thr`, `date`) VALUES
('a@p.it', 'f64551fcd6f07823cb87971cfb91446425da18286b3ab1ef935e0cbd7a69f68a', 0, '2017-06-09 09:34:30'),
('b@p.it', '3946ca64ff78d93ca61090a437cbb6b3d2ca0d488f5f9ccf3059608368b27693', 0, '2017-06-09 09:35:03'),
('c@p.it', '43bb00d0ce7790a53b91256b370c887b24791a5539a6fbfb70c5870e8c91ae5d', 0, '2017-06-09 09:36:29');

--
-- Indici per le tabelle scaricate
--

--
-- Indici per le tabelle `bid`
--
ALTER TABLE `bid`
  ADD PRIMARY KEY (`username`);

--
-- Indici per le tabelle `users`
--
ALTER TABLE `users`
  ADD PRIMARY KEY (`username`);

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
